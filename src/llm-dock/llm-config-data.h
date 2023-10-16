#ifndef LLM_CONFIG_DATA_H
#define LLM_CONFIG_DATA_H

#include <util/config-file.h>

#include <string>

struct llm_config_data {
	// local or cloud
	bool local;

	// local model path
	std::string local_model_path;

	// cloud model name
	std::string cloud_model_name;

	// cloud API key
	std::string cloud_api_key;

	// temperature
	float temperature;

	// max output tokens
	uint16_t max_output_tokens;

	// system prompt
	std::string system_prompt;

	// workflows
	std::vector<std::string> workflows;
};

// forward declaration
struct llama_context;

struct llm_global_context {
	// error message
	std::string error_message;
	// llama context
	struct llama_context *ctx_llama;
};

extern llm_config_data global_llm_config;
extern llm_global_context global_llm_context;

#define OBS_BRAIN_CONFIG_FAIL -1
#define OBS_BRAIN_CONFIG_SUCCESS 0

int saveConfig(bool create_if_not_exist = false);
int loadConfig();

#endif // LLM_CONFIG_DATA_H
