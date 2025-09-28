#include "ViewProduct.h"
#include <imgui.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif
#include "alibaba_api.h"
#include <thread>
#include <nlohmann/json.hpp>
#include <iostream>
#include "GUImodule/ProductTable.h"
#include "GUImodule/InputTextEx.h"
#include "GUImodule/PageTurner.h"

using json = nlohmann::json;

void ViewProduct::QueryProductListWithParams(const std::map<std::string, std::string>& extra_params)
{
    loading_ = true;
    errorMsg_.clear();
    pageInfoStr_.clear();
    products_.clear();

    // 直接调用AlibabaAPI中的QueryProductList
    bool ok = AlibabaAPI::QueryProductList(extra_params, products_, errorMsg_, pageInfoStr_);
    loading_ = false;
}

void ViewProduct::RenderContent()
{
    static char subject[128] = "";
    static char status[32] = "";
    static char owner_member[64] = "";
    static char display[8] = "";
    static char category_id[32] = "";
    static char group_id1[32] = "";
    static char group_id2[32] = "";
    static char group_id3[32] = "";
    static char id_list[256] = "";
    static char gmt_modified_from[32] = "";
    static char gmt_modified_to[32] = "";
    static char current_page[16] = "1";
    static char page_size[16] = "20";
    ImGui::Separator();
    ImGui::Text("自定义查询参数（可选，留空则不参与查询）");
    InputText("商品名称 subject", subject, sizeof(subject), 0, "如: 童装");
    InputText("商品状态 status", status, sizeof(status), 0, "如: on_selling");
    InputText("负责人 owner_member", owner_member, sizeof(owner_member), 0, "");
    InputText("上下架 display", display, sizeof(display), 0, "1上架 0下架");
    InputText("类目ID category_id", category_id, sizeof(category_id), 0, "");
    InputText("一级分组 group_id1", group_id1, sizeof(group_id1), 0, "");
    InputText("二级分组 group_id2", group_id2, sizeof(group_id2), 0, "");
    InputText("三级分组 group_id3", group_id3, sizeof(group_id3), 0, "");
    InputText("商品ID列表 id_list", id_list, sizeof(id_list), 0, "逗号分隔");
    InputText("最早修改时间 gmt_modified_from", gmt_modified_from, sizeof(gmt_modified_from), 0, "yyyy-MM-dd");
    InputText("最晚修改时间 gmt_modified_to", gmt_modified_to, sizeof(gmt_modified_to), 0, "yyyy-MM-dd");
    InputText("当前页 current_page", current_page, sizeof(current_page), 0, "1");
    InputText("每页大小 page_size", page_size, sizeof(page_size), 0, "20");
    if (ImGui::Button("查询我的商品"))
    {
        std::thread([this] {
            std::map<std::string, std::string> extra_params;
            #define ADD_PARAM(name, var) if (var[0] != '\0') extra_params[name] = var;
            ADD_PARAM("subject", subject)
            ADD_PARAM("status", status)
            ADD_PARAM("owner_member", owner_member)
            ADD_PARAM("display", display)
            ADD_PARAM("category_id", category_id)
            ADD_PARAM("group_id1", group_id1)
            ADD_PARAM("group_id2", group_id2)
            ADD_PARAM("group_id3", group_id3)
            ADD_PARAM("id_list", id_list)
            ADD_PARAM("gmt_modified_from", gmt_modified_from)
            ADD_PARAM("gmt_modified_to", gmt_modified_to)
            ADD_PARAM("current_page", current_page)
            ADD_PARAM("page_size", page_size)
            #undef ADD_PARAM
            this->QueryProductListWithParams(extra_params);
        }).detach();
    }
    PageTurner(current_page, [this](const char* new_page) {
        snprintf(current_page, sizeof(current_page), "%s", new_page);
        std::thread([this] {
            std::map<std::string, std::string> extra_params;
            #define ADD_PARAM(name, var) if (var[0] != '\0') extra_params[name] = var;
            ADD_PARAM("subject", subject)
            ADD_PARAM("status", status)
            ADD_PARAM("owner_member", owner_member)
            ADD_PARAM("display", display)
            ADD_PARAM("category_id", category_id)
            ADD_PARAM("group_id1", group_id1)
            ADD_PARAM("group_id2", group_id2)
            ADD_PARAM("group_id3", group_id3)
            ADD_PARAM("id_list", id_list)
            ADD_PARAM("gmt_modified_from", gmt_modified_from)
            ADD_PARAM("gmt_modified_to", gmt_modified_to)
            ADD_PARAM("current_page", current_page)
            ADD_PARAM("page_size", page_size)
            #undef ADD_PARAM
            this->QueryProductListWithParams(extra_params);
        }).detach();
    });
    if (loading_)
    {
        ImGui::Text("正在加载商品列表...");
        return;
    }
    if (!errorMsg_.empty() && errorMsg_.find_first_not_of(" \t\n\r") != std::string::npos)
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "错误: %s", errorMsg_.c_str());
    else if (!pageInfoStr_.empty())
        ImGui::Text("%s", pageInfoStr_.c_str());
    if (!products_.empty())
    {
        std::vector<ImGuiTableColumn> columns = {
            {"ID", 0}, {"名称", 0}, {"状态", 0}
        };
        std::vector<ImGuiTableRow> rows;
        for (const auto& p : products_)
            rows.push_back({std::vector<std::string>{p.id, p.name, p.status}});
        int clicked_row = ProductTable::Render("商品表格", columns, rows, 1); // 1为名称列
        if (clicked_row >= 0 && clicked_row < (int)products_.size()) {
            const std::string& url = products_[clicked_row].pc_detail_url;
            if (!url.empty()) {
                #ifdef _WIN32
                ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
                #else
                std::string cmd = std::string("xdg-open '") + url + "' &";
                system(cmd.c_str());
                #endif
            }
        }
    }
    if (products_.empty() && errorMsg_.empty())
        ImGui::Text("点击上方按钮查询商品");
}

ViewProduct::~ViewProduct() {} // 如果你有虚析构函数

const char* ViewProduct::PanelName() const { return "商品查询"; }