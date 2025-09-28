#include "AutomaticPublishing.h"
#include <imgui.h>
#include "GUImodule/InputTextEx.h"
#include "GUImodule/DropdownMenuStyle.h"
#include "alibaba_api.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <map>
#include <memory>
#include <tinyxml2.h>


// 模板数据结构
struct SchemaField {
    std::string id;
    std::string name;
    std::string type;
    std::string value;
    std::vector<SchemaField> children;
    // 新增：规则和选项
    std::vector<std::map<std::string, std::string>> rules;
    std::vector<std::pair<std::string, std::string>> options; // displayName, value
};

static std::vector<SchemaField> g_schemaFields;
static bool g_schemaReady = false;
static std::string g_schemaXmlRaw;
static std::string g_schemaError;

// 解析模板XML，提取field信息（仅支持input/singleCheck/multiCheck/label等基础类型）
static void ParseFieldNode(tinyxml2::XMLElement* fieldElem, SchemaField& f) {
    const char* id = fieldElem->Attribute("id");
    const char* name = fieldElem->Attribute("name");
    const char* type = fieldElem->Attribute("type");
    f.id = id ? id : "";
    f.name = name ? name : f.id;
    f.type = type ? type : "input";
    f.value = "";
    // 递归解析子字段
    auto* fieldsElem = fieldElem->FirstChildElement("fields");
    if (fieldsElem) {
        for (auto* subField = fieldsElem->FirstChildElement("field"); subField; subField = subField->NextSiblingElement("field")) {
            SchemaField child;
            ParseFieldNode(subField, child);
            f.children.push_back(child);
        }
    }
    // 解析规则
    auto* rulesElem = fieldElem->FirstChildElement("rules");
    if (rulesElem) {
        for (auto* ruleElem = rulesElem->FirstChildElement("rule"); ruleElem; ruleElem = ruleElem->NextSiblingElement("rule")) {
            std::map<std::string, std::string> rule;
            const tinyxml2::XMLAttribute* attr = ruleElem->FirstAttribute();
            while (attr) {
                rule[attr->Name()] = attr->Value();
                attr = attr->Next();
            }
            // 兼容 name/value/desc
            if (ruleElem->Attribute("name")) rule["name"] = ruleElem->Attribute("name");
            if (ruleElem->Attribute("value")) rule["value"] = ruleElem->Attribute("value");
            if (ruleElem->Attribute("desc")) rule["desc"] = ruleElem->Attribute("desc");
            f.rules.push_back(rule);
        }
    }
    // 解析选项
    auto* optionsElem = fieldElem->FirstChildElement("options");
    if (optionsElem) {
        for (auto* optElem = optionsElem->FirstChildElement("option"); optElem; optElem = optElem->NextSiblingElement("option")) {
            std::string displayName = optElem->Attribute("displayName") ? optElem->Attribute("displayName") : "";
            std::string value = optElem->Attribute("value") ? optElem->Attribute("value") : "";
            f.options.emplace_back(displayName, value);
        }
    }
}

static void ParseProductSchemaXml(const std::string& xml) {
    g_schemaFields.clear();
    g_schemaError.clear();
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS) {
        g_schemaError = "XML解析失败";
        return;
    }
    auto* root = doc.FirstChildElement("itemSchema");
    if (!root) { g_schemaError = "无itemSchema根节点"; return; }
    for (auto* field = root->FirstChildElement("field"); field; field = field->NextSiblingElement("field")) {
        SchemaField f;
        ParseFieldNode(field, f);
        g_schemaFields.push_back(f);
    }
}

// 统一的获取模板流程
void GetProductSchemaByCatId(const std::string& catid, const char* language) {
    std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
    std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
    std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
    std::map<std::string, std::string> apiParams;
    apiParams["cat_id"] = catid;
    apiParams["language"] = std::string(language);
    apiParams["version"] = "trade.1.1";
    std::string response, error;
    bool ok = AlibabaAPI::SyncGetProductSchema(access_token, app_key, app_secret, apiParams, response, error);
    printf("[AlibabaTool][GetProductSchema] ok=%d\n", ok ? 1 : 0);
    printf("[AlibabaTool][GetProductSchema] error: %s\n", error.c_str());
    printf("[AlibabaTool][GetProductSchema] response: %s\n", response.c_str());
    g_schemaReady = false;
    g_schemaXmlRaw.clear();
    if (ok) {
        try {
            auto j = nlohmann::json::parse(response);
            if (j.contains("alibaba_icbu_product_schema_get_response")) {
                auto& data = j["alibaba_icbu_product_schema_get_response"];
                if (data.contains("data")) {
                    g_schemaXmlRaw = data["data"].get<std::string>();
                    ParseProductSchemaXml(g_schemaXmlRaw);
                    g_schemaReady = true;
                }
            }
        } catch (...) { g_schemaError = "模板JSON解析失败"; }
    } else {
        g_schemaError = error;
    }
}

const char* AutomaticPublishing::PanelName() const { return "AutomaticPublishing"; }
void AutomaticPublishing::RenderContent() {
    static char language[16] = "zh_CN";
    static char direct_cat_id[32] = "";
    ImGui::Separator();
    ImGui::Text("直接输入cat_id获取模板:");
    ImGui::InputText("cat_id", direct_cat_id, sizeof(direct_cat_id));
    if (ImGui::Button("获取模板(直接cat_id)")) {
        std::string catid = std::string(direct_cat_id);
        if (!catid.empty()) {
            GetProductSchemaByCatId(catid, language);
        }
    }

    // 获取一级类目
    static bool loadingCategory = false;
    static std::string categoryError;
    static std::vector<std::pair<std::string, std::string>> topCategories; // id, name
    // 封装：获取指定cat_id的所有直接子类目（id, name）
    auto fetchChildCategories = [](const std::string& cat_id, const char* language, std::vector<std::pair<std::string, std::string>>& out, std::string& error) {
        out.clear();
        error.clear();
        std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
        std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
        std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
        std::map<std::string, std::string> params;
        params["language"] = language;
        params["cat_id"] = cat_id;
        std::string response, err;
        bool ok = AlibabaAPI::SyncGetCategoryList(access_token, app_key, app_secret, params, response, err);
        if (!ok) { error = err; return false; }
        try {
            auto j = nlohmann::json::parse(response);
            if (j.contains("alibaba_icbu_category_get_response")) {
                auto& cat = j["alibaba_icbu_category_get_response"]["category"];
                if (cat.contains("child_ids") && cat["child_ids"].contains("number")) {
                    for (const auto& cid : cat["child_ids"]["number"]) {
                        std::string cid_str;
                        if (cid.is_string()) cid_str = cid.get<std::string>();
                        else if (cid.is_number_integer()) cid_str = std::to_string(cid.get<int64_t>());
                        else continue;
                        // 拉取子节点详细信息
                        std::map<std::string, std::string> p2;
                        p2["language"] = language;
                        p2["cat_id"] = cid_str;
                        std::string r2, e2;
                        bool ok2 = AlibabaAPI::SyncGetCategoryList(access_token, app_key, app_secret, p2, r2, e2);
                        if (ok2) {
                            try {
                                auto j2 = nlohmann::json::parse(r2);
                                if (j2.contains("alibaba_icbu_category_get_response")) {
                                    auto& cat2 = j2["alibaba_icbu_category_get_response"]["category"];
                                    std::string id;
                                    if (cat2.contains("category_id")) {
                                        if (cat2["category_id"].is_string()) id = cat2["category_id"].get<std::string>();
                                        else if (cat2["category_id"].is_number_integer()) id = std::to_string(cat2["category_id"].get<int64_t>());
                                    }
                                    std::string name;
                                    if (cat2.contains("name")) {
                                        if (cat2["name"].is_string()) name = cat2["name"].get<std::string>();
                                        else if (cat2["name"].is_number_integer()) name = std::to_string(cat2["name"].get<int64_t>());
                                    }
                                    if (!id.empty()) out.emplace_back(id, name);
                                }
                            } catch (...) {}
                        }
                    }
                }
            }
        } catch (const std::exception& e) { error = e.what(); return false; }
        return true;
    };

    if (ImGui::Button("获取一级类目")) {
        loadingCategory = true;
        bool ok = fetchChildCategories("0", language, topCategories, categoryError);
        loadingCategory = false;
    }
    if (!categoryError.empty()) {
        ImGui::TextColored(ImVec4(1,0,0,1), "类目获取失败: %s", categoryError.c_str());
    }
    // 多级类目浏览功能
    static std::vector<std::pair<std::string, std::string>> currentCategories = topCategories;
    static std::vector<std::vector<std::pair<std::string, std::string>>> navStack; // 用于返回上一级
    static int selectedIdx = -1;
    // 若topCategories有更新，重置currentCategories（需加同步标志，避免每帧重置）
    static size_t lastTopCatCount = 0;
    if (topCategories.size() != lastTopCatCount) {
        currentCategories = topCategories;
        navStack.clear();
        selectedIdx = -1;
        lastTopCatCount = topCategories.size();
    }
    if (!currentCategories.empty()) {
        int totalCount = (int)currentCategories.size();
        int totalNameLen = 0;
        for (const auto& cat : currentCategories) totalNameLen += (int)cat.second.size();
        static DropdownMenuStyle dropdownStyle;
        dropdownStyle.PushStyle();
        std::string comboLabel;
        if (selectedIdx >= 0 && selectedIdx < (int)currentCategories.size()) {
            const auto& cat = currentCategories[selectedIdx];
            comboLabel = (cat.second.empty() ? "(无名)" : cat.second) + " | id:" + cat.first;
        } else {
            comboLabel = "类目 (数量:" + std::to_string(totalCount) + ", 名称总长:" + std::to_string(totalNameLen) + ")";
        }
        if (ImGui::BeginCombo("##类目下拉", comboLabel.c_str())) {
            for (int i = 0; i < (int)currentCategories.size(); ++i) {
                const auto& cat = currentCategories[i];
                bool is_selected = (selectedIdx == i);
                std::string itemLabel = cat.second.empty() ? ("(无名) | id:" + cat.first) : (cat.second + " | id:" + cat.first);
                if (ImGui::Selectable(itemLabel.c_str(), is_selected)) {
                    selectedIdx = i;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        dropdownStyle.PopStyle();
        // 进入下一级按钮
        if (selectedIdx >= 0 && selectedIdx < (int)currentCategories.size()) {
            if (ImGui::Button("进入下一级")) {
                const std::string& selectedId = currentCategories[selectedIdx].first;
                std::vector<std::pair<std::string, std::string>> nextCategories;
                std::string err;
                bool ok = fetchChildCategories(selectedId, language, nextCategories, err);
                if (ok && !nextCategories.empty()) {
                    navStack.push_back(currentCategories);
                    currentCategories = nextCategories;
                    selectedIdx = -1;
                } else {
                    // 末端子叶，直接获取模板
                    GetProductSchemaByCatId(selectedId, language);
                }
            }
        }
        // 返回上一级按钮
        if (!navStack.empty() && ImGui::Button("返回上一级")) {
            currentCategories = navStack.back();
            navStack.pop_back();
            selectedIdx = -1;
        }
    }
    // 渲染模板自定义表单
    if (g_schemaReady) {
        ImGui::Separator();
        ImGui::Text("商品发布模板：");
        if (!g_schemaError.empty()) {
            ImGui::TextColored(ImVec4(1,0,0,1), "模板解析失败: %s", g_schemaError.c_str());
        }
        static std::map<std::string, std::string> fieldValues;

        std::function<void(std::vector<SchemaField>&, int)> RenderFields;
        RenderFields = [&](std::vector<SchemaField>& fields, int depth) {
            for (auto& f : fields) {
                ImGui::PushID(f.id.c_str());
                ImGui::Indent(depth * 20.0f);
                ImGui::Text("%s:", f.name.c_str());
                // 显示规则提示
                std::string tips;
                for (const auto& rule : f.rules) {
                    if (rule.count("tipRule")) tips += std::string("提示: ") + rule.at("tipRule") + "\n";
                    if (rule.count("requiredRule") && rule.at("requiredRule") == "true") tips += "必填项\n";
                    if (rule.count("maxLengthRule")) tips += "最大长度: " + rule.at("maxLengthRule") + "\n";
                    if (rule.count("minInputNumRule")) tips += "最小输入数: " + rule.at("minInputNumRule") + "\n";
                    if (rule.count("maxInputNumRule")) tips += "最大输入数: " + rule.at("maxInputNumRule") + "\n";
                    if (rule.count("valueTypeRule")) tips += "类型: " + rule.at("valueTypeRule") + "\n";
                    if (rule.count("desc")) tips += rule.at("desc") + "\n";
                }
                if (!tips.empty()) {
                    ImGui::TextColored(ImVec4(0.7f,0.7f,0.1f,1), "%s", tips.c_str());
                }
                char buf[256] = {0};
                auto it = fieldValues.find(f.id);
                if (it != fieldValues.end()) strncpy(buf, it->second.c_str(), sizeof(buf)-1);
                if (f.type == "input") {
                    if (ImGui::InputText("##input", buf, sizeof(buf))) {
                        fieldValues[f.id] = buf;
                    }
                } else if (f.type == "complex" || f.type == "multiComplex") {
                    if (!f.children.empty()) {
                        RenderFields(f.children, depth+1);
                    }
                } else {
                    // 其他类型可扩展
                    if (ImGui::InputText("##other", buf, sizeof(buf))) {
                        fieldValues[f.id] = buf;
                    }
                }
                ImGui::Unindent(depth * 20.0f);
                ImGui::PopID();
            }
        };
        RenderFields(g_schemaFields, 0);

        if (ImGui::Button("提交模板内容")) {
            // 递归组装为XML，保留原schema结构
            std::function<void(const std::vector<SchemaField>&, std::string&)> BuildXml;
            BuildXml = [&](const std::vector<SchemaField>& fields, std::string& xml) {
                for (const auto& f : fields) {
                    xml += "<field id=\"" + f.id + "\" type=\"" + f.type + "\">";
                    if (!f.children.empty()) {
                        xml += "<fields>";
                        BuildXml(f.children, xml);
                        xml += "</fields>";
                    }
                    xml += "<value>" + fieldValues[f.id] + "</value></field>";
                }
            };
            std::string xml = "<itemSchema>";
            BuildXml(g_schemaFields, xml);
            xml += "</itemSchema>";
            printf("[AlibabaTool][SubmitSchema] xml: %s\n", xml.c_str());

            // 对接阿里巴巴国际站商品发布API
            std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
            std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
            std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
            std::map<std::string, std::string> apiParams;
            apiParams["param_product_top_publish_request"] = xml;
            apiParams["language"] = language;
            apiParams["version"] = "trade.1.1";
            std::string response, error;
            bool ok = AlibabaAPI::SyncAddProduct(access_token, app_key, app_secret, xml, response, error);
            if (ok) {
                ImGui::TextColored(ImVec4(0,1,0,1), "商品发布成功！");
            } else {
                ImGui::TextColored(ImVec4(1,0,0,1), "商品发布失败: %s", error.c_str());
            }
            printf("[AlibabaTool][AddProduct] ok=%d\n", ok ? 1 : 0);
            printf("[AlibabaTool][AddProduct] error: %s\n", error.c_str());
            printf("[AlibabaTool][AddProduct] response: %s\n", response.c_str());
        }
    }

    // 图片银行查询测试
    ImGui::Separator();
    ImGui::Text("图片银行查询接口测试 (alibaba.icbu.photobank.list)");
    static char photobank_location_type[32] = "ALL_GROUP";
    static char photobank_page_size[16] = "10";
    static char photobank_current_page[16] = "1";
    static char photobank_group_id[32] = "";
    static char photobank_extra_context[64] = "";
    static std::string photobank_response;
    static std::string photobank_error;

    ImGui::InputText("location_type", photobank_location_type, sizeof(photobank_location_type));
    ImGui::InputText("page_size", photobank_page_size, sizeof(photobank_page_size));
    ImGui::InputText("current_page", photobank_current_page, sizeof(photobank_current_page));
    ImGui::InputText("group_id", photobank_group_id, sizeof(photobank_group_id));
    ImGui::InputText("extra_context", photobank_extra_context, sizeof(photobank_extra_context));

    if (ImGui::Button("查询图片银行")) {
        std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
        std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
        std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
        std::map<std::string, std::string> params;
        if (strlen(photobank_location_type) > 0) params["location_type"] = photobank_location_type;
        if (strlen(photobank_page_size) > 0) params["page_size"] = photobank_page_size;
        if (strlen(photobank_current_page) > 0) params["current_page"] = photobank_current_page;
        if (strlen(photobank_group_id) > 0) params["group_id"] = photobank_group_id;
        if (strlen(photobank_extra_context) > 0) params["extra_context"] = photobank_extra_context;

        photobank_response.clear();
        photobank_error.clear();
        bool ok = AlibabaAPI::SyncQueryPhotobankList(
            access_token,
            app_key,
            app_secret,
            photobank_response,
            photobank_error,
            params
        );
        if (ok) {
            ImGui::TextColored(ImVec4(0,1,0,1), "查询成功！");
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "查询失败: %s", photobank_error.c_str());
        }
    }
    if (!photobank_response.empty()) {
        ImGui::Text("图片银行返回内容：");
        ImGui::InputTextMultiline("photobank_response", (char*)photobank_response.c_str(), photobank_response.size() + 1, ImVec2(400, 120), ImGuiInputTextFlags_ReadOnly);
    }

    // 图片分组查询测试
    ImGui::Separator();
    ImGui::Text("图片分组查询接口测试 (alibaba.icbu.photobank.group.list)");
    static char photobank_group_id_param[32] = "";
    static char photobank_group_extra_context[64] = "";
    static std::string photobank_group_response;
    static std::string photobank_group_error;

    ImGui::InputText("分组id(id)", photobank_group_id_param, sizeof(photobank_group_id_param));
    ImGui::InputText("extra_context", photobank_group_extra_context, sizeof(photobank_group_extra_context));

    if (ImGui::Button("查询图片分组")) {
        std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
        std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
        std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
        std::map<std::string, std::string> params;
        if (strlen(photobank_group_id_param) > 0) params["id"] = photobank_group_id_param;
        if (strlen(photobank_group_extra_context) > 0) params["extra_context"] = photobank_group_extra_context;

        photobank_group_response.clear();
        photobank_group_error.clear();
        bool ok = AlibabaAPI::SyncQueryPhotobankGroupList(
            access_token,
            app_key,
            app_secret,
            photobank_group_response,
            photobank_group_error,
            params
        );
        if (ok) {
            ImGui::TextColored(ImVec4(0,1,0,1), "分组查询成功！");
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "分组查询失败: %s", photobank_group_error.c_str());
        }
    }
    if (!photobank_group_response.empty()) {
        ImGui::Text("图片分组返回内容：");
        ImGui::InputTextMultiline("photobank_group_response", (char*)photobank_group_response.c_str(), photobank_group_response.size() + 1, ImVec2(400, 120), ImGuiInputTextFlags_ReadOnly);

        // 可选：格式化展示分组列表
        try {
            auto j = nlohmann::json::parse(photobank_group_response);
            if (j.contains("alibaba_icbu_photobank_group_list_response") &&
                j["alibaba_icbu_photobank_group_list_response"].contains("photobank_group_list") &&
                j["alibaba_icbu_photobank_group_list_response"]["photobank_group_list"].contains("photobank_group_do")) {

                auto& groups = j["alibaba_icbu_photobank_group_list_response"]["photobank_group_list"]["photobank_group_do"];
                ImGui::Separator();
                ImGui::Text("分组列表（前10条）：");
                int count = 0;
                for (const auto& g : groups) {
                    if (++count > 10) break;
                    std::string info = std::to_string(count) + ". " +
                        g.value("name", "") + " | id: " +
                        g.value("id", "") + " | 图片数: " +
                        std::to_string(g.value("image_count", 0));
                    ImGui::TextWrapped("%s", info.c_str());
                }
            }
        } catch (...) {
            ImGui::TextColored(ImVec4(1,0,0,1), "分组结果解析失败");
        }
    }

}