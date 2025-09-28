#ifndef DEEPSEEK_API_H
#define DEEPSEEK_API_H

#include <string>
#include "nlohmann/json.hpp"

class DeepSeekAPI {
public:
    DeepSeekAPI();
    ~DeepSeekAPI();

    // prompt为json字符串，apiKey为你的DeepSeek密钥
    nlohmann::json sendRequest(const std::string& prompt, const std::string& apiKey);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // DEEPSEEK_API_H