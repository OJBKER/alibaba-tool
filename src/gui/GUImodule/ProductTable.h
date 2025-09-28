#pragma once
#include <vector>
#include <string>
#include <imgui.h>

struct ImGuiTableColumn {
    std::string label;
    float width = 0.0f; // 0为自适应
};

struct ImGuiTableRow {
    std::vector<std::string> cells;
};

class ProductTable {
public:
    // 渲染美化表格，返回被点击（选中）的行索引，未选中返回-1
    static int Render(const char* table_id, const std::vector<ImGuiTableColumn>& columns, const std::vector<ImGuiTableRow>& rows, int selectable_col = 1);
};
