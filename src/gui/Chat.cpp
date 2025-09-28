#include "Chat.h"
#include <imgui.h>
#include "deepseek_api.h"
#include "alibaba_api.h"
#include <iostream>
#include <nlohmann/json.hpp>

void Chat::RenderContent() {
    static nlohmann::json apiResult;      // 保存API执行结果
    static std::string apiError;          // 保存API错误
    static bool apiExecuted = false;      // 是否已执行API

    static char systemPrompt[2048] =
        "你可以调用本页的 AlibabaAPI 类中的静态函数来访问阿里巴巴国际站的相关接口。使用时请给出函数名和参数，参数需按注释说明填写。例如：\n"
        "- 获取 Access Token：{\"GetAccessToken\": {\"code\": \"\", \"access_token\": \"\", \"errorMsg\": \"\"}}\n"
        "- 查询商品列表：{\"SyncQueryProductList\": {\"response_out\": \"\", \"error_out\": \"\", \"extra_params\": {}}}\n"
        "- 发布商品：{\"SyncPublishProduct\": {\"params\": {}, \"response_out\": \"\", \"error_out\": \"\"}}\n"
        "- 获取商品发布模板：{\"SyncGetProductSchema\": {\"params\": {}, \"response_out\": \"\", \"error_out\": \"\"}}\n"
        "- 正式发布商品：{\"SyncAddProduct\": {\"param_product_top_publish_request_xml\": \"\", \"response_out\": \"\", \"error_out\": \"\"}}\n"
        "- 获取类目列表：{\"SyncGetCategoryList\": {\"params\": {}, \"response_out\": \"\", \"error_out\": \"\"}}\n"
        "所有调用请以 JSON 格式输出，包含函数名和参数列表，与使用者对话时则将内容输入到talk字段比如\"talk\": \"Hello!\""
        "我将会把api的返回结果给你，你要为其做评价，并且返回内容输入到字段\"talk\""
    ;

    // 对话历史只保留 systemPrompt、最近一次 user 和 assistant
    static std::vector<nlohmann::json> messageHistory;
    if (messageHistory.empty() || messageHistory[0]["role"] != "system" || messageHistory[0]["content"] != systemPrompt) {
        messageHistory.clear();
        messageHistory.push_back({{"role", "system"}, {"content", systemPrompt}});
    }

    static char userContent[1024] = "Hello!";
    ImGui::InputTextMultiline("系统提示", systemPrompt, sizeof(systemPrompt), ImVec2(400, 80));
    ImGui::InputTextMultiline("输入内容", userContent, sizeof(userContent), ImVec2(400, 80));

    // 显示历史对话
    std::string historyText;
    for (const auto& msg : messageHistory) {
        historyText += "[" + msg["role"].get<std::string>() + "]: " + msg["content"].get<std::string>() + "\n";
    }
    ImGui::InputTextMultiline("对话历史", (char*)historyText.c_str(), historyText.size() + 1, ImVec2(400, 100), ImGuiInputTextFlags_ReadOnly);

    // DeepSeek 测试按钮
    static std::string deepseekResult;
    static bool apiShouldExecute = false; // 标记是否需要执行API
    static bool apiResultAddedToHistory = false;

    if (!apiResult.is_null() && !apiResult.empty() && !apiResultAddedToHistory) {
        std::string apiResultStr = apiResult.dump(2);
        if (messageHistory.size() > 2) messageHistory.resize(2);
        messageHistory.push_back({
            {"role", "assistant"},
            {"content", apiResultStr}
        });
        apiResultAddedToHistory = true;
    }

    if (ImGui::Button("测试 DeepSeek 聊天接口")) {
        try {
            DeepSeekAPI api;
            // 不清理 messageHistory，直接用完整上下文
            messageHistory.push_back({{"role", "user"}, {"content", userContent}});
            nlohmann::json req = {
                {"model", "deepseek-chat"},
                {"messages", messageHistory},
                {"stream", false},
                {"temperature", 0.0},
                {"response_format", {{"type", "json_object"}}}
            };
            deepseekResult = api.sendRequest(req.dump(), "sk-f89f740d07ad4e0d829243e8bbe1dbd4").dump(2);

            auto respJson = nlohmann::json::parse(deepseekResult);
            if (respJson.contains("choices") && respJson["choices"].is_array() && !respJson["choices"].empty()) {
                auto& choice = respJson["choices"][0];
                if (choice.contains("message") && choice["message"].contains("content")) {
                    std::string contentText = choice["message"]["content"];
                    messageHistory.push_back({{"role", "assistant"}, {"content", contentText}});
                }
            }
            apiShouldExecute = true;
            apiResultAddedToHistory = false; // 新请求后允许再次添加API结果
        } catch (const std::exception& e) {
            deepseekResult = std::string("Error: ") + e.what();
            apiShouldExecute = false;
            apiResultAddedToHistory = false;
        }
    }

    // 只在新请求后执行一次API
    static std::string lastDeepseekResult;
    if (apiShouldExecute && !deepseekResult.empty() && deepseekResult != lastDeepseekResult) {
        std::string contentText, matchedFunction;
        apiResult = nlohmann::json::object();
        apiError.clear();
        apiExecuted = false;
        try {
            auto respJson = nlohmann::json::parse(deepseekResult);
            if (respJson.contains("choices") && respJson["choices"].is_array() && !respJson["choices"].empty()) {
                auto& choice = respJson["choices"][0];
                if (choice.contains("message") && choice["message"].contains("content")) {
                    contentText = choice["message"]["content"];
                    auto contentJson = nlohmann::json::parse(contentText);
                    for (int i = 0; i <= static_cast<int>(AlibabaApiFunction::QueryProductList); ++i) {
                        AlibabaApiFunction func = static_cast<AlibabaApiFunction>(i);
                        std::string funcName = AlibabaApiFunctionToString(func);
                        if (contentJson.contains(funcName)) {
                            apiExecuted = ExecuteAlibabaApiFunction(func, contentJson[funcName], apiResult, apiError);
                            break;
                        }
                    }
                }
            }
        } catch (...) {
            apiError = "API解析或执行失败";
        }
        lastDeepseekResult = deepseekResult;
        apiShouldExecute = false;
    }

    // 始终显示API执行结果
    ImGui::InputTextMultiline("返回内容", (char*)deepseekResult.c_str(), deepseekResult.size() + 1, ImVec2(400, 200), ImGuiInputTextFlags_ReadOnly);
    if (!apiResult.is_null() && !apiResult.empty()) {
        std::string apiResultStr = apiResult.dump(2);
        ImGui::InputTextMultiline("API执行结果", (char*)apiResultStr.c_str(), apiResultStr.size() + 1, ImVec2(400, 120), ImGuiInputTextFlags_ReadOnly);
        if (!apiError.empty()) {
            ImGui::TextColored(ImVec4(1,0,0,1), "API错误: %s", apiError.c_str());
        }
        // 将API执行结果添加到上下文（messageHistory）
        if (messageHistory.size() > 2) messageHistory.resize(2);
        messageHistory.push_back({
            {"role", "assistant"},
            {"content", apiResultStr}
        });
    }
}
