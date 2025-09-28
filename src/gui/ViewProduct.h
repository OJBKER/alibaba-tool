#pragma once
#include "GUImodule/ContentPanelBase.h"
#include "alibaba_api.h"
#include <vector>
#include <string>
#include <map>

class ViewProduct : public ContentPanelBase {
public:
    virtual ~ViewProduct(); // 添加虚析构函数
    const char* PanelName() const override;
    void RenderContent() override;
    // 查询商品列表
    void QueryProductList();
    // 支持自定义参数的商品查询
    void QueryProductListWithParams(const std::map<std::string, std::string>& extra_params);
    // 商品数据结构
    struct ProductInfo {
        std::string id;
        std::string name;
        std::string status;
        std::string pc_detail_url;
    };
    std::vector<AlibabaAPI::ProductInfo> products_; // 修改为AlibabaAPI::ProductInfo
    std::string errorMsg_;
    std::string pageInfoStr_;
    bool loading_ = false;
};