
#include "llama-inference.h"
#include "plugin-support.h"
#include "llm-config-data.h"

#include <obs-module.h>

#include <vector>
#include <future>
#include <string>
#include <algorithm>
#include <sstream>
#include <cmath>

std::string replace(const std::string &s, const std::string &from, const std::string &to)
{
	std::string result = s;
	size_t pos = 0;
	while ((pos = result.find(from, pos)) != std::string::npos) {
		result.replace(pos, from.length(), to);
		pos += to.length();
	}
	return result;
}

std::string get_system_info(const llama_context_params &params)
{
	std::ostringstream os;

	os << "system_info: n_threads = " << params.n_threads;
	os << " (n_threads_batch = " << params.n_threads_batch << ")";
	os << " / " << std::thread::hardware_concurrency() << " | " << llama_print_system_info();

	return os.str();
}

std::vector<llama_token> llama_tokenize(const struct llama_model *model, const std::string &text,
					bool add_bos)
{
	// upper limit for the number of tokens
	uint64_t n_tokens = text.length() + (uint64_t)add_bos;
	std::vector<llama_token> result(n_tokens);
	n_tokens = llama_tokenize(model, text.data(), (int)text.length(), result.data(),
				  (int)result.size(), add_bos);
	if (n_tokens < 0) {
		result.resize(std::abs((int)n_tokens));
		int check = llama_tokenize(model, text.data(), (int)text.length(), result.data(),
					   (int)result.size(), add_bos);
		GGML_ASSERT(check == (int)std::abs((int)n_tokens));
	} else {
		result.resize(n_tokens);
	}
	return result;
}

std::vector<llama_token> llama_tokenize(const struct llama_context *ctx, const std::string &text,
					bool add_bos)
{
	return ::llama_tokenize(llama_get_model(ctx), text, add_bos);
}

std::string llama_token_to_piece(const struct llama_context *ctx, llama_token token)
{
	std::vector<char> result(8, 0);
	const int n_tokens = llama_token_to_piece(llama_get_model(ctx), token, result.data(),
						  (int)result.size());
	if (n_tokens < 0) {
		result.resize(-n_tokens);
		int check = llama_token_to_piece(llama_get_model(ctx), token, result.data(),
						 (int)result.size());
		GGML_ASSERT(check == (int)-n_tokens);
	} else {
		result.resize(n_tokens);
	}

	return std::string(result.data(), result.size());
}

struct llama_context *llama_init_context(const std::string &model_file_path)
{
	llama_backend_init(true);

	// initialize the model
	struct llama_model_params mparams = llama_model_default_params();

	struct llama_model *model_llama =
		llama_load_model_from_file(model_file_path.c_str(), mparams);

	if (model_llama == nullptr) {
		obs_log(LOG_ERROR, "%s: error: unable to load model\n", __func__);
		return nullptr;
	}

	// get model details using llama_model_desc
	char model_desc[128];
	llama_model_desc(model_llama, model_desc, sizeof(model_desc));
	obs_log(LOG_INFO, "%s: model_desc = %s", __func__, model_desc);

	// initialize the context
	struct llama_context_params lparams = llama_context_default_params();

	struct llama_context *ctx_llama = llama_new_context_with_model(model_llama, lparams);

	if (ctx_llama == nullptr) {
		obs_log(LOG_ERROR, "%s: error: failed to create the llama_context", __func__);
		return nullptr;
	}

	if (llama_get_model(ctx_llama) == nullptr) {
		obs_log(LOG_ERROR, "%s: error: failed to get model from llama_context", __func__);
		return nullptr;
	}

	obs_log(LOG_INFO, "%s", get_system_info(lparams).c_str());

	// Warm up in another thread
	std::thread t([ctx_llama, lparams]() {
		obs_log(LOG_INFO, "warming up the model with an empty run");

		std::vector<llama_token> tokens_list = {
			llama_token_bos(ctx_llama),
			llama_token_eos(ctx_llama),
		};

		llama_decode(ctx_llama, llama_batch_get_one(tokens_list.data(),
							    (int)std::min(tokens_list.size(),
									  (size_t)lparams.n_batch),
							    0, 0));
		llama_kv_cache_tokens_rm(ctx_llama, -1, -1);
		llama_reset_timings(ctx_llama);

		obs_log(LOG_INFO, "warmed up the model");
	});
	t.detach();

	return ctx_llama;
}

std::string llama_inference(const std::string &promptIn, struct llama_context *ctx,
			    std::function<void(const std::string &)> partial_generation_callback)
{
	std::string output = "";

	// tokenize the prompt
	// replace {0} in the system prompt with the prompt
	std::string prompt = replace(global_llm_config.system_prompt, "{0}", promptIn);

	std::vector<llama_token> tokens_list;
	tokens_list = ::llama_tokenize(ctx, prompt, true);

	// total length of the sequence including the prompt
	const int n_len = 512;

	const int n_ctx = llama_n_ctx(ctx);
	const int n_kv_req = (int)(tokens_list.size() + (n_len - tokens_list.size()));

	obs_log(LOG_INFO, "%s: n_len = %d, n_ctx = %d, n_kv_req = %d", __func__, n_len, n_ctx,
		n_kv_req);

	// make sure the KV cache is big enough to hold all the prompt and generated tokens
	if (n_kv_req > n_ctx) {
		obs_log(LOG_INFO,
			"%s: error: n_kv_req > n_ctx, the required KV cache size is not big enough",
			__func__);
		obs_log(LOG_INFO, "%s:        either reduce n_parallel or increase n_ctx",
			__func__);
		return "";
	}

	// create a llama_batch with size 512
	// we use this object to submit token data for decoding

	llama_batch batch = llama_batch_init(512, 0);

	// evaluate the initial prompt
	batch.n_tokens = (int)tokens_list.size();

	for (int32_t i = 0; i < batch.n_tokens; i++) {
		batch.token[i] = tokens_list[i];
		batch.pos[i] = i;
		batch.seq_id[i] = 0;
		batch.logits[i] = false;
	}

	// llama_decode will output logits only for the last token of the prompt
	batch.logits[batch.n_tokens - 1] = true;

	if (llama_decode(ctx, batch) != 0) {
		obs_log(LOG_INFO, "%s: llama_decode() failed\n", __func__);
		return "";
	}

	// main loop

	int n_cur = batch.n_tokens;
	int n_decode = 0;

	const auto t_main_start = ggml_time_us();

	while (n_cur <= n_len) {
		// sample the next token
		{
			auto n_vocab = llama_n_vocab(llama_get_model(ctx));
			auto *logits = llama_get_logits_ith(ctx, batch.n_tokens - 1);

			std::vector<llama_token_data> candidates;
			candidates.reserve(n_vocab);

			for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
				candidates.emplace_back(
					llama_token_data{token_id, logits[token_id], 0.0f});
			}

			llama_token_data_array candidates_p = {candidates.data(), candidates.size(),
							       false};

			// sample the most likely token
			const llama_token new_token_id =
				llama_sample_token_greedy(ctx, &candidates_p);

			// is it an end of stream?
			if (new_token_id == llama_token_eos(ctx) || n_cur == n_len) {
				break;
			}

			partial_generation_callback(llama_token_to_piece(ctx, new_token_id));

			// prepare the next batch
			batch.n_tokens = 0;

			// push this new token for next evaluation
			batch.token[batch.n_tokens] = new_token_id;
			batch.pos[batch.n_tokens] = n_cur;
			batch.seq_id[batch.n_tokens] = 0;
			batch.logits[batch.n_tokens] = true;

			batch.n_tokens += 1;

			n_decode += 1;
		}

		n_cur += 1;

		// evaluate the current batch with the transformer model
		if (llama_decode(ctx, batch)) {
			obs_log(LOG_ERROR, "%s : failed to eval, return code %d", __func__, 1);
			return "";
		}
	}

	// reset the KV cache
	llama_kv_cache_tokens_rm(ctx, -1, -1);
	llama_reset_timings(ctx);

	const auto t_main_end = ggml_time_us();

	obs_log(LOG_INFO, "%s: decoded %d tokens in %.2f s, speed: %.2f t/s", __func__, n_decode,
		(t_main_end - t_main_start) / 1000000.0f,
		n_decode / ((t_main_end - t_main_start) / 1000000.0f));

	return output;
}
