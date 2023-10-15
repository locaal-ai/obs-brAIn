#include "llm-config-data.h"
#include "plugin-support.h"

#include <obs-module.h>
#include <string>
#include <nlohmann/json.hpp>

llm_config_data global_llm_config;
llm_global_context global_llm_context;

void config_defaults()
{
	const std::string LLAMA_DEFAULT_SYSTEM_PROMPT = R"([INST] <<SYS>>
You are a helpful, respectful, positive, safe and honest assistant.
Don't include harmful, unethical, racist, sexist, toxic, dangerous, socially biased, untruthful or illegal content.
<</SYS>> Q: {0} [/INST] A:)";

	global_llm_config.local = true;
	global_llm_config.local_model_path = "";
	global_llm_config.cloud_model_name = "";
	global_llm_config.cloud_api_key = "";
	global_llm_config.temperature = 0.9f;
	global_llm_config.max_output_tokens = 64;
	global_llm_config.system_prompt = LLAMA_DEFAULT_SYSTEM_PROMPT;
}

void create_config_folder()
{
	char *config_folder_path = obs_module_config_path("");
	if (config_folder_path == nullptr) {
		obs_log(LOG_ERROR, "Failed to get config folder path");
		return;
	}
	std::filesystem::path config_folder_std_path(config_folder_path);
	bfree(config_folder_path);

	// create the folder if it doesn't exist
	if (!std::filesystem::exists(config_folder_std_path)) {
#ifdef _WIN32
		obs_log(LOG_INFO, "Config folder does not exist, creating: %S",
			config_folder_std_path.c_str());
#else
		obs_log(LOG_INFO, "Config folder does not exist, creating: %s",
			config_folder_std_path.c_str());
#endif
		// Create the config folder
		std::filesystem::create_directories(config_folder_std_path);
	}
}

int getConfig(config_t **config, bool create_if_not_exist = false)
{
	create_config_folder(); // ensure the config folder exists

	// Get the config file
	char *config_file_path = obs_module_config_path("config.ini");

	int ret = config_open(config, config_file_path,
			      create_if_not_exist ? CONFIG_OPEN_ALWAYS : CONFIG_OPEN_EXISTING);
	if (ret != CONFIG_SUCCESS) {
		obs_log(LOG_INFO, "Failed to open config file %s", config_file_path);
		return OBS_BRAIN_CONFIG_FAIL;
	}

	return OBS_BRAIN_CONFIG_SUCCESS;
}

std::string llm_config_data_to_json(const llm_config_data &data);
llm_config_data llm_config_data_from_json(const std::string &json);

int saveConfig(bool create_if_not_exist)
{
	config_t *config_file;
	if (getConfig(&config_file, create_if_not_exist) == OBS_BRAIN_CONFIG_SUCCESS) {
		std::string json = llm_config_data_to_json(global_llm_config);
		config_set_string(config_file, "general", "llm_config", json.c_str());
		config_save(config_file);
		config_close(config_file);
		return OBS_BRAIN_CONFIG_SUCCESS;
	}
	return OBS_BRAIN_CONFIG_FAIL;
}

int loadConfig()
{
	config_t *config_file;
	if (getConfig(&config_file) == OBS_BRAIN_CONFIG_SUCCESS) {
		const char *json = config_get_string(config_file, "general", "llm_config");
		if (json != nullptr) {
			global_llm_config = llm_config_data_from_json(json);
			config_close(config_file);
			return OBS_BRAIN_CONFIG_SUCCESS;
		}
		config_close(config_file);
	} else {
		obs_log(LOG_WARNING, "Failed to load config file. Creating a new one.");
		config_defaults();
		if (saveConfig(true) == OBS_BRAIN_CONFIG_SUCCESS) {
			obs_log(LOG_INFO, "Saved default LLM settings");
			return OBS_BRAIN_CONFIG_SUCCESS;
		} else {
			obs_log(LOG_ERROR, "Failed to save LLM settings");
		}
	}
	return OBS_BRAIN_CONFIG_FAIL;
}

// serialize llm_config_data to a json string
std::string llm_config_data_to_json(const llm_config_data &data)
{
	nlohmann::json j;
	j["local"] = data.local;
	j["local_model_path"] = data.local_model_path;
	j["cloud_model_name"] = data.cloud_model_name;
	j["cloud_api_key"] = data.cloud_api_key;
	j["temperature"] = data.temperature;
	j["max_output_tokens"] = data.max_output_tokens;
	j["system_prompt"] = data.system_prompt;
	return j.dump();
}

// deserialize llm_config_data from a json string
llm_config_data llm_config_data_from_json(const std::string &json)
{
	nlohmann::json j = nlohmann::json::parse(json);
	llm_config_data data;
	data.local = j["local"];
	data.local_model_path = j["local_model_path"];
	data.cloud_model_name = j["cloud_model_name"];
	data.cloud_api_key = j["cloud_api_key"];
	data.temperature = j["temperature"];
	data.max_output_tokens = j["max_output_tokens"];
	data.system_prompt = j["system_prompt"];
	return data;
}
