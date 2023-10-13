#include <llama.h>

#include <string>

struct llama_context *llama_init_context(const std::string &model_file_path);

std::string llama_inference(const std::string &prompt, struct llama_context *ctx,
  std::function<void(const std::string &)> partial_generation_callback);

extern const std::string LLAMA_DEFAULT_SYSTEM_PROMPT;
