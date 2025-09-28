#include "MainContent.h"
#include <imgui.h>
#include "GUImodule/InputTextEx.h"
#include "GUImodule/my_popup.h"
#include "alibaba_api.h"
#include <string>
#include <windows.h> // ShellExecuteA
#include <iostream>  // std::cout
#include <fstream>
#include <nlohmann/json.hpp>

const char* MainContent::PanelName() const { return "MainContent"; }

//使用本地配置设置
void LoadAppConfig(std::string& app_key, std::string& app_secret, std::string& code, std::string& access_token) {
    std::ifstream fin("app_config.json");
    if (!fin) return;
    nlohmann::json j;
    fin >> j;
    app_key = j.value("app_key", "");
    app_secret = j.value("app_secret", "");
    code = j.value("code", "");
    access_token = j.value("access_token", "");
    // 同步到last_response_map
    AlibabaAPI::last_response_map.clear();
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.value().is_string())
            AlibabaAPI::last_response_map[it.key()] = it.value();
        else
            AlibabaAPI::last_response_map[it.key()] = it.value().dump();
    }
}
// 保存App配置到本地文件
void SaveAppConfig(const std::string& app_key, const std::string& app_secret, const std::string& code, const std::string& access_token) {
    nlohmann::json j;
    j["app_key"] = app_key;
    j["app_secret"] = app_secret;
    j["code"] = code;
    j["access_token"] = access_token;
    std::ofstream fout("app_config.json");
    fout << j.dump(2);
}

void MainContent::RenderContent() {
    static char code[256] = "";
    static bool getting_token = false;
    static std::string errorMsg;
    static bool loaded = false;
    static char access_token_buf[1024] = "";
    // 取出已保存的app_key/app_secret/access_token
    std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
    std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
    std::string saved_access_token;
    if (!loaded) {
        std::string saved_key, saved_secret, saved_code;
        LoadAppConfig(saved_key, saved_secret, saved_code, saved_access_token);
        if (!saved_key.empty()) app_key = saved_key;
        if (!saved_secret.empty()) app_secret = saved_secret;
        if (!saved_code.empty()) strncpy(code, saved_code.c_str(), sizeof(code)-1);
        if (!saved_access_token.empty()) strncpy(access_token_buf, saved_access_token.c_str(), sizeof(access_token_buf)-1);
        AlibabaAPI::config_map["app_key"] = app_key;
        AlibabaAPI::config_map["app_secret"] = app_secret;
        loaded = true;
    }
    char app_key_buf[128] = "";
    char app_secret_buf[128] = "";
    strncpy(app_key_buf, app_key.c_str(), sizeof(app_key_buf) - 1);
    strncpy(app_secret_buf, app_secret.c_str(), sizeof(app_secret_buf) - 1);

    ImGui::Text("阿里巴巴国际站 OAuth2 测试");

    if (InputText("App Key", app_key_buf, sizeof(app_key_buf))) {
        AlibabaAPI::config_map["app_key"] = app_key_buf;
    }
    if (InputText("App Secret", app_secret_buf, sizeof(app_secret_buf), ImGuiInputTextFlags_Password)) {
        AlibabaAPI::config_map["app_secret"] = app_secret_buf;
    }
    if (ImGui::Button("保存App信息")) {
        AlibabaAPI::SetAppCredentials(app_key_buf, app_secret_buf);
        SaveAppConfig(app_key_buf, app_secret_buf, code, access_token_buf);
        MyPopup::Show("提示", "App Key 和 App Secret 已保存");
    }
    ImGui::Separator();

    if (ImGui::Button("打开授权页面获取 code")) {
        std::string auth_url =
            "https://open-api.alibaba.com/oauth/authorize?response_type=code&force_auth=true"
            "&redirect_uri=https%3A%2F%2Fwww.alibaba.com"
            "&client_id=" + AlibabaAPI::config_map["app_key"];
        std::cout << "[auth_url] " << auth_url << std::endl;
        ShellExecuteA(NULL, "open", auth_url.c_str(), NULL, NULL, SW_SHOWNORMAL);
        MyPopup::Show("提示", "请在浏览器授权并复制回调URL中的 code 到下方输入框。");
    }

    InputText("授权code", code, sizeof(code));
    if (ImGui::Button("用 code 换取 access_token") && code[0] != '\0' && !getting_token) {
        getting_token = true;
        std::string token, err;
        bool success = AlibabaAPI::GetAccessToken(
            AlibabaAPI::config_map["app_key"], AlibabaAPI::config_map["app_secret"], code, token, err
        );
        if (success) {
            strncpy(access_token_buf, token.c_str(), sizeof(access_token_buf)-1);
            SaveAppConfig(AlibabaAPI::config_map["app_key"], AlibabaAPI::config_map["app_secret"], code, access_token_buf);
            MyPopup::Show("成功", "access_token 获取成功！");
        } else {
            MyPopup::Show("失败", (err + "\n原始响应: " + token).c_str());
        }
        getting_token = false;
    }
    InputText("access_token", access_token_buf, sizeof(access_token_buf), ImGuiInputTextFlags_ReadOnly);

    // 渲染弹窗
    MyPopup::RenderPopup();
}