#ifndef ALIBABA_API_H
#define ALIBABA_API_H

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>


enum class AlibabaApiFunction {
    GetAccessToken,
    RefreshAccessToken,
    SetAppCredentials,
    GetAppCredentials,
    SyncQueryProductList,
    SyncPublishProduct,
    SyncGetProductSchema,
    SyncAddProduct,
    SyncGetCategoryList,
    QueryProductList
};

// 枚举转字符串
inline const char* AlibabaApiFunctionToString(AlibabaApiFunction func) {
    switch (func) {
        case AlibabaApiFunction::GetAccessToken: return "GetAccessToken";
        case AlibabaApiFunction::RefreshAccessToken: return "RefreshAccessToken";
        case AlibabaApiFunction::SetAppCredentials: return "SetAppCredentials";
        case AlibabaApiFunction::GetAppCredentials: return "GetAppCredentials";
        case AlibabaApiFunction::SyncQueryProductList: return "SyncQueryProductList";
        case AlibabaApiFunction::SyncPublishProduct: return "SyncPublishProduct";
        case AlibabaApiFunction::SyncGetProductSchema: return "SyncGetProductSchema";
        case AlibabaApiFunction::SyncAddProduct: return "SyncAddProduct";
        case AlibabaApiFunction::SyncGetCategoryList: return "SyncGetCategoryList";
        case AlibabaApiFunction::QueryProductList: return "QueryProductList";
        default: return "";
    }
}

class AlibabaAPI
{
public:
    // 获取 Access Token（新版带签名）
    // 参数说明：
    //  appKey：应用Key
    //  appSecret：应用密钥
    //  partnerId：调用者partner_id
    //  code：授权码
    //  redirect_uri：重定向地址
    //  errorMsg：错误消息（失败时返回）
    // 返回true成功，false失败
    static bool GetAccessToken(const std::string& appKey,
                                  const std::string& appSecret,                                 
                                  const std::string& code,                                 
                                  std::string& access_token,
                                  std::string& errorMsg,
                                  std::map<std::string, std::string>* all_response = nullptr);
    // 刷新 Access Token（新版带签名）
    // 参数说明：
    //  appKey：应用Key
    //  appSecret：应用密钥
    //  refresh_token：刷新令牌
    //  errorMsg：错误消息（失败时返回）
    // 返回true成功，false失败
    static bool RefreshAccessToken(const std::string& appKey,
                                   const std::string& appSecret,
                                   const std::string& refresh_token,
                                   std::string& access_token,
                                   std::string& errorMsg,
                                   std::map<std::string, std::string>* all_response = nullptr);

    // 设置App Key和App Secret
    static void SetAppCredentials(const std::string& appKey, const std::string& appSecret);
    // 获取App Key和App Secret
    static void GetAppCredentials(std::string& appKey, std::string& appSecret);

    // 新增：同步风格商品查询API
    static bool SyncQueryProductList(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        std::string& response_out,
        std::string& error_out,
        const std::map<std::string, std::string>& extra_params = {} // 可选扩展参数
    );


    // 新增：同步风格商品发布API
    // params: 商品参数（subject, category_id, price, quantity, description, image_urls等）
    // debug_curl: 可选，调试输出curl命令
    static bool SyncPublishProduct(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        const std::map<std::string, std::string>& params,
        std::string& response_out,
        std::string& error_out,
        std::string* debug_curl = nullptr
    );

        // 获取商品发布schema模板（alibaba.icbu.product.schema.get）
    static bool SyncGetProductSchema(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        const std::map<std::string, std::string>& params,
        std::string& response_out,
        std::string& error_out
    );


    // 新增：同步风格商品正式发布API（alibaba.icbu.product.schema.add）
    static bool SyncAddProduct(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        const std::string& param_product_top_publish_request_xml,
        std::string& response_out,
        std::string& error_out,
        std::string* debug_curl = nullptr
    );

    // 获取类目列表（alibaba.icbu.category.get）
    static bool SyncGetCategoryList(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        const std::map<std::string, std::string>& params,
        std::string& response_out,
        std::string& error_out
    );

    // 新增：同步风格图片银行查询接口（alibaba.icbu.photobank.list）
    static bool SyncQueryPhotobankList(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        std::string& response_out,
        std::string& error_out,
        const std::map<std::string, std::string>& extra_params = {} // 可选扩展参数
    );

    // 新增：同步风格图片分组查询接口（alibaba.icbu.photobank.group.list）
    static bool SyncQueryPhotobankGroupList(
        const std::string& session,
        const std::string& app_key,
        const std::string& app_secret,
        std::string& response_out,
        std::string& error_out,
        const std::map<std::string, std::string>& extra_params = {} // 可选扩展参数
    );

    // 全局响应参数map
    static std::map<std::string, std::string> config_map;
    static std::map<std::string, std::string> last_response_map;

    struct ProductInfo {
        std::string id;
        std::string name;
        std::string status;
        std::string pc_detail_url;
    };

    // 查询商品列表并解析为ProductInfo结构体
    static bool QueryProductList(
        const std::map<std::string, std::string>& extra_params,
        std::vector<ProductInfo>& products,
        std::string& errorMsg,
        std::string& pageInfoStr
    );
};



std::string generateSign(const std::string& apiPath, const std::string& appSecret, const std::map<std::string, std::string>& params);
std::string generateSyncSign(const std::map<std::string, std::string> &params, const std::string &appSecret);

// 根据枚举值和json参数执行对应API函数
bool ExecuteAlibabaApiFunction(
    AlibabaApiFunction func,
    const nlohmann::json& params,
    nlohmann::json& result,
    std::string& errorMsg
);


#endif // ALIBABA_API_H
