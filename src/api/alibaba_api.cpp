#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <openssl/sha.h>  // OpenSSL SHA256
#include <openssl/hmac.h> // OpenSSL HMAC
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include "alibaba_api.h"

using json = nlohmann::json;

// 定义全局配置map
std::map<std::string, std::string> AlibabaAPI::config_map;
std::map<std::string, std::string> AlibabaAPI::last_response_map;

// 辅助函数，去除字符串首尾空白
static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

void AlibabaAPI::SetAppCredentials(const std::string &appKey, const std::string &appSecret)
{
    config_map["app_key"] = trim(appKey);
    config_map["app_secret"] = trim(appSecret);
}

void AlibabaAPI::GetAppCredentials(std::string &appKey, std::string &appSecret)
{
    appKey = config_map.count("app_key") ? config_map["app_key"] : "";
    appSecret = config_map.count("app_secret") ? config_map["app_secret"] : "";
}

// 回调函数，接收curl返回内容
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// 生成签名：新平台注册API签名规范，使用HMAC-SHA256
std::string generateSign(
    const std::string &apiPath,
    const std::string &appSecret,
    const std::map<std::string, std::string> &params)
{
    // 1. 排序参数（ASCII升序）
    std::vector<std::string> keys;
    for (const auto &p : params)
    {
        if (p.first != "sign" && !p.first.empty())
            keys.push_back(p.first);
    }
    std::sort(keys.begin(), keys.end());

    // 2. 拼接参数名与参数值
    std::string toSign;
    for (const auto &key : keys)
    {
        toSign += key + params.at(key);
    }
    // 3. 添加API名称至拼接字符串开头
    toSign = apiPath + toSign;

    // 4. HMAC-SHA256签名
    unsigned char hmac[32];
    unsigned int hmac_len = 0;
    HMAC(EVP_sha256(), appSecret.c_str(), appSecret.length(),
         (const unsigned char *)toSign.c_str(), toSign.size(), hmac, &hmac_len);

    // 5. 转大写HEX
    std::stringstream ss;
    for (int i = 0; i < 32; ++i)
    {
        ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)hmac[i];
    }

    return ss.str();
}

// sync风格签名：不添加API名称至拼接字符串开头，其余与generateSign一致
std::string generateSyncSign(
    const std::map<std::string, std::string> &params,
    const std::string &appSecret)
{
    // 1. 排序参数（ASCII升序）
    std::vector<std::string> keys;
    for (const auto &p : params)
    {
        if (p.first != "sign" && !p.first.empty())
            keys.push_back(p.first);
    }
    std::sort(keys.begin(), keys.end());
    // 2. 拼接参数名与参数值（不加API路径），支持object类型（如param_product_top_publish_request为JSON字符串）
    std::string toSign;
    for (const auto &key : keys)
    {
        const std::string &val = params.at(key);
        toSign += key + val;
    }
    // 输出拼接后的签名字符串到终端
    std::cout << "[generateSyncSign] toSign: " << toSign << std::endl;
    // 3. HMAC-SHA256签名
    unsigned char hmac[32];
    unsigned int hmac_len = 0;
    HMAC(EVP_sha256(), appSecret.c_str(), appSecret.length(),
         (const unsigned char *)toSign.c_str(), toSign.size(), hmac, &hmac_len);
    // 4. 转大写HEX
    std::stringstream ss;
    for (int i = 0; i < 32; ++i)
    {
        ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)hmac[i];
    }
    return ss.str();
}

// 获取AccessToken示例函数
bool AlibabaAPI::GetAccessToken(
    const std::string &appKey,
    const std::string &appSecret,
    const std::string &code,
    std::string &access_token,
    std::string &errorMsg,
    std::map<std::string, std::string> *all_response)
{
    const std::string api_path = "/auth/token/create";
    const std::string endpoint = "https://open-api.alibaba.com/rest" + api_path;

    // 公共参数和业务参数
    std::map<std::string, std::string> params;
    params["app_key"] = appKey;
    params["code"] = code;
    params["sign_method"] = "sha256";
    params["simplify"] = "true";
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    params["timestamp"] = std::to_string(millis);

    // 生成签名
    std::string sign = generateSign(api_path, appSecret, params);
    params["sign"] = sign;

    // 组装POST body参数（application/json）
    json body_json;
    for (const auto &p : params)
    {
        body_json[p.first] = p.second;
    }
    std::string postFields = body_json.dump();

    // 实际请求(POST)
    CURL *curl2 = curl_easy_init();
    if (!curl2)
    {
        errorMsg = "CURL 初始化失败";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl2, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl2, CURLOPT_POST, 1L);
    curl_easy_setopt(curl2, CURLOPT_POSTFIELDS, postFields.c_str());
    curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl2, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl2, CURLOPT_CAINFO, "cacert.pem");
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");
    curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, headers);
    CURLcode res = curl_easy_perform(curl2);
    long http_code = 0;
    curl_easy_getinfo(curl2, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl2);

    if (res != CURLE_OK)
    {
        errorMsg = std::string("curl_easy_perform失败: ") + curl_easy_strerror(res);
        return false;
    }
    try
    {
        auto resp_json = json::parse(response);
        // 存储所有响应参数
        std::map<std::string, std::string> *target_map = all_response ? all_response : &AlibabaAPI::last_response_map;
        if (target_map)
        {
            target_map->clear();
            for (auto it = resp_json.begin(); it != resp_json.end(); ++it)
            {
                if (it.value().is_string())
                    (*target_map)[it.key()] = it.value();
                else
                    (*target_map)[it.key()] = it.value().dump();
            }
        }
        // 打印所有响应参数
        for (auto it = resp_json.begin(); it != resp_json.end(); ++it)
        {
            std::cout << "[响应] " << it.key() << ": " << it.value() << std::endl;
        }
        if (http_code == 200 && resp_json.contains("access_token"))
        {
            access_token = resp_json["access_token"].get<std::string>();
            return true;
        }
        else
        {
            errorMsg = resp_json.value("message", "未知错误: " + response);
            return false;
        }
    }
    catch (const std::exception &e)
    {
        errorMsg = std::string("JSON解析失败: ") + e.what() + "\n响应内容:\n" + response;
        return false;
    }
}

// 刷新AccessToken函数
bool AlibabaAPI::RefreshAccessToken(
    const std::string &appKey,
    const std::string &appSecret,
    const std::string &refresh_token,
    std::string &access_token,
    std::string &errorMsg,
    std::map<std::string, std::string> *all_response)
{
    const std::string api_path = "/auth/token/refresh";
    const std::string endpoint = "https://open-api.alibaba.com/rest" + api_path;

    // 公共参数和业务参数
    std::map<std::string, std::string> params;
    params["app_key"] = appKey;
    params["refresh_token"] = refresh_token;
    params["sign_method"] = "sha256";
    params["simplify"] = "true";
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    params["timestamp"] = std::to_string(millis);

    // 生成签名
    std::string sign = generateSign(api_path, appSecret, params);
    params["sign"] = sign;

    // 组装POST body参数（application/json）
    json body_json;
    for (const auto &p : params)
    {
        body_json[p.first] = p.second;
    }
    std::string postFields = body_json.dump();

    // 实际请求(POST)
    CURL *curl2 = curl_easy_init();
    if (!curl2)
    {
        errorMsg = "CURL 初始化失败";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl2, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl2, CURLOPT_POST, 1L);
    curl_easy_setopt(curl2, CURLOPT_POSTFIELDS, postFields.c_str());
    curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl2, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl2, CURLOPT_CAINFO, "cacert.pem");
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");
    curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, headers);
    CURLcode res = curl_easy_perform(curl2);
    long http_code = 0;
    curl_easy_getinfo(curl2, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl2);

    if (res != CURLE_OK)
    {
        errorMsg = std::string("curl_easy_perform失败: ") + curl_easy_strerror(res);
        return false;
    }
    try
    {
        auto resp_json = json::parse(response);
        // 存储所有响应参数
        std::map<std::string, std::string> *target_map = all_response ? all_response : &AlibabaAPI::last_response_map;
        if (target_map)
        {
            target_map->clear();
            for (auto it = resp_json.begin(); it != resp_json.end(); ++it)
            {
                if (it.value().is_string())
                    (*target_map)[it.key()] = it.value();
                else
                    (*target_map)[it.key()] = it.value().dump();
            }
        }
        if (http_code == 200 && resp_json.contains("access_token"))
        {
            access_token = resp_json["access_token"].get<std::string>();
            return true;
        }
        else
        {
            errorMsg = resp_json.value("message", "未知错误: " + response);
            return false;
        }
    }
    catch (const std::exception &e)
    {
        errorMsg = std::string("JSON解析失败: ") + e.what() + "\n响应内容:\n" + response;
        return false;
    }
}

// 新增：同步风格商品查询API，返回响应内容和错误信息
bool AlibabaAPI::SyncQueryProductList(
    const std::string &session,
    const std::string &app_key,
    const std::string &app_secret,
    std::string &response_out,
    std::string &error_out,
    const std::map<std::string, std::string> &extra_params // 可选扩展参数
)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    std::map<std::string, std::string> params;
    // 必填参数
    params["language"] = "ENGLISH";
    params["app_key"] = app_key;
    params["sign_method"] = "sha256";
    params["method"] = "alibaba.icbu.product.list";
    params["session"] = session;
    // 自动生成timestamp
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    params["timestamp"] = std::to_string(millis);
    // 可选参数（全部列出，便于扩展调试）
    params["id_list"] = "";
    params["owner_member"] = "";
    params["status"] = "approved";
    params["display"] = "";
    params["category_id"] = "";
    params["current_page"] = "1";
    params["page_size"] = "10";
    params["subject"] = "";
    params["group_id3"] = "";
    params["group_id2"] = "";
    params["group_id1"] = "";
    params["id"] = "";
    params["gmt_modified_to"] = "";
    params["gmt_modified_from"] = "";
    // 合并extra_params覆盖默认参数
    for (const auto &kv : extra_params)
    {
        params[kv.first] = kv.second;
    }
    // 移除所有value为空的字段（不参与签名和请求）
    for (auto it = params.cbegin(); it != params.cend();)
    {
        if (it->second.empty())
            params.erase(it++);
        else
            ++it;
    }
    // 生成sync风格签名
    std::string sign = generateSyncSign(params, app_secret);
    params["sign"] = sign;
    // 拼接query
    std::string query;
    for (auto it = params.begin(); it != params.end(); ++it)
    {
        if (it != params.begin())
            query += "&";
        query += it->first + "=" + it->second;
    }
    std::string url = base_url + "?" + query;
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        error_out = "CURL init failed";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t
                     {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb; });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
    {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    }
    else
    {
        response_out = response;
        return true;
    }
}

// 新增：同步风格商品发布API，POST JSON，支持调试输出curl
bool AlibabaAPI::SyncPublishProduct(
    const std::string &session,
    const std::string &app_key,
    const std::string &app_secret,
    const std::map<std::string, std::string> &params,
    std::string &response_out,
    std::string &error_out,
    std::string *debug_curl)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    // 1. 组装主参数
    std::map<std::string, std::string> all_params;
    all_params["app_key"] = app_key;
    all_params["sign_method"] = "sha256";
    all_params["method"] = "alibaba.icbu.product.schema.add.draft";
    all_params["access_token"] = session;
    // 自动生成timestamp
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    all_params["timestamp"] = std::to_string(millis);
    // param_product_top_publish_request参数，需urlencode
    nlohmann::json publish_req;
    for (const auto &kv : params)
    {
        publish_req[kv.first] = kv.second;
    }
    std::string publish_req_str = publish_req.dump();
    // urlencode（只对param_product_top_publish_request）
    auto urlencode = [](const std::string &s) -> std::string
    {
        std::ostringstream oss;
        for (unsigned char c : s)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
                oss << c;
            else
                oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return oss.str();
    };
    all_params["param_product_top_publish_request"] = urlencode(publish_req_str);
    // 2. 生成签名（所有主参数，不含body）
    std::string sign = generateSyncSign(all_params, app_secret);
    all_params["sign"] = sign;
    // 3. 构造POST JSON体（所有主参数，param_product_top_publish_request为urlencode字符串）
    nlohmann::json body;
    for (const auto &kv : all_params)
    {
        body[kv.first] = kv.second;
    }
    std::string post_data = body.dump();
    // 4. 调试输出curl命令
    if (debug_curl)
    {
        *debug_curl = "curl -X POST --header 'Content-Type: application/json;charset=utf-8' --data '" + post_data + "' '" + base_url + "'";
    }
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        error_out = "CURL init failed";
        return false;
    }
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t
                     {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb; });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
    {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    }
    else
    {
        response_out = response;
        return true;
    }
}

// 获取商品发布schema模板（alibaba.icbu.product.schema.get）
bool AlibabaAPI::SyncGetProductSchema(
    const std::string &session,
    const std::string &app_key,
    const std::string &app_secret,
    const std::map<std::string, std::string> &params,
    std::string &response_out,
    std::string &error_out)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    // 1. 构造param_product_top_publish_request业务参数
    nlohmann::json reqObj;
    if (params.count("cat_id")) reqObj["cat_id"] = params.at("cat_id");
    if (params.count("language")) reqObj["language"] = params.at("language");
    if (params.count("version")) reqObj["version"] = params.at("version");
    std::string param_request = reqObj.dump();
    // 2. 构造主参数map（只包含官方字段）
    std::map<std::string, std::string> all_params;
    all_params["app_key"] = app_key;
    all_params["sign_method"] = "sha256";
    all_params["method"] = "alibaba.icbu.product.schema.get";
    all_params["session"] = session;
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    all_params["timestamp"] = std::to_string(millis);
    all_params["param_product_top_publish_request"] = param_request;
    // 3. 移除所有value为空的字段（不参与签名和请求）
    for (auto it = all_params.cbegin(); it != all_params.cend();)
    {
        if (it->second.empty())
            all_params.erase(it++);
        else
            ++it;
    }
    // 4. 生成签名
    std::string sign = generateSyncSign(all_params, app_secret);
    all_params["sign"] = sign;
    // 5. 拼接query（param_product_top_publish_request保持原始JSON字符串，不编码）
    std::string query;
    for (auto it = all_params.begin(); it != all_params.end(); ++it)
    {
        if (it != all_params.begin())
            query += "&";
        query += it->first + "=" + it->second;
    }
    std::string url = base_url + "?" + query;
    // 6. 调试输出签名字符串和最终URL
    std::cout << "[SyncGetProductSchema] 签名字符串: ";
    std::vector<std::string> keys;
    for (const auto &p : all_params) {
        if (p.first != "sign" && !p.first.empty()) keys.push_back(p.first);
    }
    std::sort(keys.begin(), keys.end());
    std::string toSign;
    for (const auto &key : keys) {
        toSign += key + all_params.at(key);
    }
    std::cout << toSign << std::endl;
    std::cout << "[SyncGetProductSchema] 最终URL: " << url << std::endl;
    // 7. 实际请求
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        error_out = "CURL init failed";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t
                     {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb; });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
    {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    }
    else
    {
        response_out = response;
        return true;
    }
}
// 获取类目列表（alibaba.icbu.category.get）
bool AlibabaAPI::SyncGetCategoryList(
    const std::string &session,
    const std::string &app_key,
    const std::string &app_secret,
    const std::map<std::string, std::string> &params,
    std::string &response_out,
    std::string &error_out)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    std::map<std::string, std::string> all_params;
    all_params["app_key"] = app_key;
    all_params["sign_method"] = "sha256";
    all_params["method"] = "alibaba.icbu.category.get";
    all_params["access_token"] = session;
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    all_params["timestamp"] = std::to_string(millis);
    // 合并参数
    for (const auto &kv : params)
    {
        all_params[kv.first] = kv.second;
    }
    // 移除空字段
    for (auto it = all_params.cbegin(); it != all_params.cend();)
    {
        if (it->second.empty())
            all_params.erase(it++);
        else
            ++it;
    }
    // 生成签名
    std::string sign = generateSyncSign(all_params, app_secret);
    all_params["sign"] = sign;
    // 拼接query
    std::string query;
    for (auto it = all_params.begin(); it != all_params.end(); ++it)
    {
        if (it != all_params.begin())
            query += "&";
        query += it->first + "=" + it->second;
    }
    std::string url = base_url + "?" + query;
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        error_out = "CURL init failed";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t
                     {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb; });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
    {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    }
    else
    {
        response_out = response;
        return true;
    }
}

// 新增：同步风格商品正式发布API（alibaba.icbu.product.schema.add）
bool AlibabaAPI::SyncAddProduct(
    const std::string &session,
    const std::string &app_key,
    const std::string &app_secret,
    const std::string &param_product_top_publish_request_xml, // 递归生成的XML字符串
    std::string &response_out,
    std::string &error_out,
    std::string *debug_curl)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    // 1. 组装param_product_top_publish_request对象
    // 1. 组装param_product_top_publish_request对象，直接用原始JSON字符串
    // 你可以根据需要传递完整对象，这里举例
    std::string publish_req_str = std::string("{\"cat_id\":\"201929802\",\"language\":\"zh_CN\",\"version\":\"trade.1.1\"}");
    // 2. 组装主参数
    std::map<std::string, std::string> all_params;
    all_params["app_key"] = app_key;
    all_params["sign_method"] = "sha256";
    all_params["method"] = "alibaba.icbu.product.schema.add";
    all_params["access_token"] = session;
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    all_params["timestamp"] = std::to_string(millis);
    all_params["param_product_top_publish_request"] = publish_req_str;
    // 3. 生成签名（所有主参数，不含body）
    std::string sign = generateSyncSign(all_params, app_secret);
    all_params["sign"] = sign;
    // 4. 构造POST表单体（application/x-www-form-urlencoded），不做任何转义
    std::ostringstream oss;
    for (auto it = all_params.begin(); it != all_params.end(); ++it) {
        if (it != all_params.begin()) oss << "&";
        oss << it->first << "=" << it->second;
    }
    std::string post_data = oss.str();
    // 5. 调试输出curl命令和最终POST数据
    std::string curl_cmd = "curl -X POST --header 'Content-Type: application/x-www-form-urlencoded' --data '" + post_data + "' '" + base_url + "'";
    if (debug_curl) {
        *debug_curl = curl_cmd;
    }
    CURL *curl = curl_easy_init();
    if (!curl) {
        error_out = "CURL init failed";
        return false;
    }
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    } else {
        response_out = response;
        return true;
    }
}

// 查询商品列表（alibaba.icbu.product.list）
bool AlibabaAPI::QueryProductList(
    const std::map<std::string, std::string>& extra_params,
    std::vector<ProductInfo>& products,
    std::string& errorMsg,
    std::string& pageInfoStr
) {
    products.clear();
    errorMsg.clear();
    pageInfoStr.clear();

    std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
    std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
    std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
    if (access_token.empty() || app_key.empty() || app_secret.empty()) {
        errorMsg = "请先获取access_token、app_key和app_secret";
        return false;
    }
    std::string response, error;
    if (!AlibabaAPI::SyncQueryProductList(access_token, app_key, app_secret, response, error, extra_params)) {
        errorMsg = error;
        return false;
    }
    try {
        auto resp_json = nlohmann::json::parse(response);
        const nlohmann::json* result = nullptr;
        if (resp_json.contains("result"))
            result = &resp_json["result"];
        else if (resp_json.contains("alibaba_icbu_product_list_response"))
            result = &resp_json["alibaba_icbu_product_list_response"];
        else
            result = &resp_json;
        int current_page = result->value("current_page", 0);
        int page_size = result->value("page_size", 0);
        int total_item = result->value("total_item", 0);
        char pageInfo[128];
        snprintf(pageInfo, sizeof(pageInfo), "当前页: %d, 每页: %d, 总数: %d", current_page, page_size, total_item);
        pageInfoStr = pageInfo;
        // 解析商品列表
        const nlohmann::json* products_json = nullptr;
        if (result->contains("products"))
            products_json = &(*result)["products"];
        if (products_json) {
            const nlohmann::json* arr = products_json;
            if (products_json->is_object() && products_json->contains("alibaba_product_brief_response"))
                arr = &(*products_json)["alibaba_product_brief_response"];
            if (arr->is_array()) {
                for (const auto& item : *arr) {
                    ProductInfo info;
                    info.id = item.value("product_id", item.value("productId", ""));
                    info.name = item.value("subject", "");
                    info.status = item.value("status", "");
                    info.pc_detail_url = item.value("pc_detail_url", item.value("pcDetailUrl", ""));
                    products.push_back(info);
                }
            }
        }
        if (resp_json.contains("error_message"))
            errorMsg = resp_json["error_message"].get<std::string>();
    } catch (const std::exception& e) {
        errorMsg = std::string("JSON解析失败: ") + e.what();
        return false;
    }
    return true;
}

bool ExecuteAlibabaApiFunction(
    AlibabaApiFunction func,
    const nlohmann::json& params,
    nlohmann::json& result,
    std::string& errorMsg
) {
    bool ok = false;
    result = nlohmann::json::object();

    // 从 config_map 和 last_response_map 获取必要参数
    std::string appKey = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
    std::string appSecret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
    std::string session = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";

    switch (func) {
        case AlibabaApiFunction::GetAccessToken: {
            std::string code = params.value("code", "");
            std::string access_token, err;
            ok = AlibabaAPI::GetAccessToken(appKey, appSecret, code, access_token, err);
            result["access_token"] = access_token;
            errorMsg = err;
            break;
        }
        case AlibabaApiFunction::RefreshAccessToken: {
            std::string refresh_token = params.value("refresh_token", "");
            std::string access_token, err;
            ok = AlibabaAPI::RefreshAccessToken(appKey, appSecret, refresh_token, access_token, err);
            result["access_token"] = access_token;
            errorMsg = err;
            break;
        }
        case AlibabaApiFunction::SetAppCredentials: {
            std::string newAppKey = params.value("appKey", "");
            std::string newAppSecret = params.value("appSecret", "");
            AlibabaAPI::SetAppCredentials(newAppKey, newAppSecret);
            ok = true;
            break;
        }
        case AlibabaApiFunction::GetAppCredentials: {
            std::string outAppKey, outAppSecret;
            AlibabaAPI::GetAppCredentials(outAppKey, outAppSecret);
            result["appKey"] = outAppKey;
            result["appSecret"] = outAppSecret;
            ok = true;
            break;
        }
        case AlibabaApiFunction::SyncQueryProductList: {
            std::string response, error;
            std::map<std::string, std::string> extra_params;
            if (params.contains("extra_params") && params["extra_params"].is_object()) {
                for (auto it = params["extra_params"].begin(); it != params["extra_params"].end(); ++it)
                    extra_params[it.key()] = it.value();
            }
            ok = AlibabaAPI::SyncQueryProductList(session, appKey, appSecret, response, error, extra_params);
            result["response"] = response;
            errorMsg = error;
            break;
        }
        case AlibabaApiFunction::SyncPublishProduct: {
            std::string response, error;
            std::map<std::string, std::string> publish_params;
            if (params.contains("params") && params["params"].is_object()) {
                for (auto it = params["params"].begin(); it != params["params"].end(); ++it)
                    publish_params[it.key()] = it.value();
            }
            ok = AlibabaAPI::SyncPublishProduct(session, appKey, appSecret, publish_params, response, error);
            result["response"] = response;
            errorMsg = error;
            break;
        }
        case AlibabaApiFunction::SyncGetProductSchema: {
            std::string response, error;
            std::map<std::string, std::string> schema_params;
            if (params.contains("params") && params["params"].is_object()) {
                for (auto it = params["params"].begin(); it != params["params"].end(); ++it)
                    schema_params[it.key()] = it.value();
            }
            ok = AlibabaAPI::SyncGetProductSchema(session, appKey, appSecret, schema_params, response, error);
            result["response"] = response;
            errorMsg = error;
            break;
        }
        case AlibabaApiFunction::SyncAddProduct: {
            std::string xml = params.value("param_product_top_publish_request_xml", "");
            std::string response, error;
            ok = AlibabaAPI::SyncAddProduct(session, appKey, appSecret, xml, response, error);
            result["response"] = response;
            errorMsg = error;
            break;
        }
        case AlibabaApiFunction::SyncGetCategoryList: {
            std::string response, error;
            std::map<std::string, std::string> cat_params;
            if (params.contains("params") && params["params"].is_object()) {
                for (auto it = params["params"].begin(); it != params["params"].end(); ++it)
                    cat_params[it.key()] = it.value();
            }
            ok = AlibabaAPI::SyncGetCategoryList(session, appKey, appSecret, cat_params, response, error);
            result["response"] = response;
            errorMsg = error;
            break;
        }
        case AlibabaApiFunction::QueryProductList: {
            std::vector<AlibabaAPI::ProductInfo> products;
            std::string err, pageInfo;
            std::map<std::string, std::string> extra_params;
            if (params.is_object()) {
                for (auto it = params.begin(); it != params.end(); ++it)
                    extra_params[it.key()] = it.value();
            }
            ok = AlibabaAPI::QueryProductList(extra_params, products, err, pageInfo);
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& p : products) {
                arr.push_back({{"id", p.id}, {"name", p.name}, {"status", p.status}, {"pc_detail_url", p.pc_detail_url}});
            }
            result["products"] = arr;
            result["pageInfo"] = pageInfo;
            errorMsg = err;
            break;
        }
        default:
            errorMsg = "未知API函数";
            break;
    }
    return ok;
}

// 国际站图片银行查询接口（alibaba.icbu.photobank.list）
bool AlibabaAPI::SyncQueryPhotobankList(
    const std::string& session,
    const std::string& app_key,
    const std::string& app_secret,
    std::string& response_out,
    std::string& error_out,
    const std::map<std::string, std::string>& extra_params // 可选扩展参数
)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    std::map<std::string, std::string> params;
    // 必填参数
    params["app_key"] = app_key;
    params["sign_method"] = "sha256";
    params["method"] = "alibaba.icbu.photobank.list";
    params["session"] = session;
    // 自动生成timestamp
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    params["timestamp"] = std::to_string(millis);

    // 可选参数
    // extra_context: Object（如icvId），直接传字符串
    if (extra_params.count("extra_context")) params["extra_context"] = extra_params.at("extra_context");
    // location_type: String
    if (extra_params.count("location_type")) params["location_type"] = extra_params.at("location_type");
    // page_size: Number
    if (extra_params.count("page_size")) params["page_size"] = extra_params.at("page_size");
    // current_page: Number
    if (extra_params.count("current_page")) params["current_page"] = extra_params.at("current_page");
    // group_id: String
    if (extra_params.count("group_id")) params["group_id"] = extra_params.at("group_id");

    // 生成sync风格签名
    std::string sign = generateSyncSign(params, app_secret);
    params["sign"] = sign;

    // 拼接query
    std::string query;
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (it != params.begin()) query += "&";
        query += it->first + "=" + it->second;
    }
    std::string url = base_url + "?" + query;

    CURL* curl = curl_easy_init();
    if (!curl) {
        error_out = "CURL init failed";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    } else {
        response_out = response;
        return true;
    }
}

// 查询图片分组接口（alibaba.icbu.photobank.group.list）
bool AlibabaAPI::SyncQueryPhotobankGroupList(
    const std::string& session,
    const std::string& app_key,
    const std::string& app_secret,
    std::string& response_out,
    std::string& error_out,
    const std::map<std::string, std::string>& extra_params // 可选扩展参数
)
{
    std::string base_url = "https://open-api.alibaba.com/sync";
    std::map<std::string, std::string> params;
    // 必填参数
    params["app_key"] = app_key;
    params["sign_method"] = "sha256";
    params["method"] = "alibaba.icbu.photobank.group.list";
    params["session"] = session;
    // 自动生成timestamp
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    params["timestamp"] = std::to_string(millis);

    // 可选参数
    // extra_context: Object（如icvId），直接传字符串
    if (extra_params.count("extra_context")) params["extra_context"] = extra_params.at("extra_context");
    // id: Number（分组ID）
    if (extra_params.count("id")) params["id"] = extra_params.at("id");

    // 生成sync风格签名
    std::string sign = generateSyncSign(params, app_secret);
    params["sign"] = sign;

    // 拼接query
    std::string query;
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (it != params.begin()) query += "&";
        query += it->first + "=" + it->second;
    }
    std::string url = base_url + "?" + query;

    CURL* curl = curl_easy_init();
    if (!curl) {
        error_out = "CURL init failed";
        return false;
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        error_out = std::string("CURL error: ") + curl_easy_strerror(res);
        return false;
    } else {
        response_out = response;
        return true;
    }
}