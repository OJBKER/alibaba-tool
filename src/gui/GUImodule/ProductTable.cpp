#include "ProductTable.h"
#include <imgui.h>

int ProductTable::Render(const char* table_id, const std::vector<ImGuiTableColumn>& columns, const std::vector<ImGuiTableRow>& rows, int selectable_col) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 winBg = style.Colors[ImGuiCol_WindowBg];
    float bg_luminance = 0.299f * winBg.x + 0.587f * winBg.y + 0.114f * winBg.z;
    bool dark_theme = bg_luminance < 0.5f;
    ImVec4 headerColor, headerTextColor, rowBg1, rowBg2, borderColor, hoverColor, textColor;
    if (dark_theme) {
        headerColor = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        headerTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        rowBg1 = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
        rowBg2 = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        borderColor = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        hoverColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        headerColor = ImVec4(0.13f, 0.32f, 0.55f, 1.0f);
        headerTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        rowBg1 = ImVec4(0.97f, 0.98f, 1.0f, 1.0f);
        rowBg2 = ImVec4(0.92f, 0.95f, 1.0f, 1.0f);
        borderColor = ImVec4(0.75f, 0.85f, 0.95f, 1.0f);
        hoverColor = ImVec4(0.80f, 0.88f, 1.0f, 1.0f);
        textColor = ImVec4(0.10f, 0.12f, 0.18f, 1.0f);
    }
    float borderThickness = 1.0f;
    float rounding = 6.0f;

    int clicked_row = -1;
    if (ImGui::BeginTable(table_id, (int)columns.size(), ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_HighlightHoveredColumn)) {
        // Header
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, headerColor);
        ImGui::PushStyleColor(ImGuiCol_Text, headerTextColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // 加粗字体（如有）
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].width > 0)
                ImGui::TableSetupColumn(columns[i].label.c_str(), 0, columns[i].width);
            else
                ImGui::TableSetupColumn(columns[i].label.c_str());
        }
        ImGui::TableHeadersRow();
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        // Rows
        for (size_t row = 0; row < rows.size(); ++row) {
            ImGui::TableNextRow();
            ImVec4 bg = (row % 2 == 0) ? rowBg1 : rowBg2;
            for (size_t col = 0; col < columns.size(); ++col) {
                ImGui::TableSetColumnIndex((int)col);
                bool hovered = ImGui::IsItemHovered();
                ImVec4 cell_bg = hovered ? hoverColor : bg;
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                ImGui::PushStyleColor(ImGuiCol_TableRowBg, cell_bg);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
                if ((int)col == selectable_col) {
                    ImGui::PushID((int)row);
                    if (ImGui::Selectable(col < rows[row].cells.size() ? rows[row].cells[col].c_str() : "", false, ImGuiSelectableFlags_SpanAllColumns))
                        clicked_row = (int)row;
                    ImGui::PopID();
                } else {
                    ImGui::TextUnformatted(col < rows[row].cells.size() ? rows[row].cells[col].c_str() : "");
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }
        }
        // 绘制自定义边框
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 table_min = ImGui::GetCursorScreenPos();
        ImVec2 table_max = ImVec2(table_min.x + ImGui::GetContentRegionAvail().x, table_min.y + ImGui::GetContentRegionAvail().y);
        draw_list->AddRect(table_min, table_max, ImGui::ColorConvertFloat4ToU32(borderColor), rounding, 0, borderThickness);
        ImGui::EndTable();
    }
    return clicked_row;
}
