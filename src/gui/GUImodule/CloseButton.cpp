#include "CloseButton.h"

bool CloseButton::Render(float /*offset_x*/, float height)
{
    const float btn_width = 50.0f;
    const float btn_height = height;

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 oldColor = style.Colors[ImGuiCol_Button];
    ImVec4 oldHovered = style.Colors[ImGuiCol_ButtonHovered];
    ImVec4 oldActive = style.Colors[ImGuiCol_ButtonActive];

    // 默认透明背景，悬停/按下为红色
    style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.1f, 0.1f, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, (btn_height - ImGui::GetFontSize()) * 0.5f));
    bool clicked = ImGui::Button("关闭##CloseBtn", ImVec2(btn_width, btn_height));
    ImGui::PopStyleVar(2);

    // 恢复颜色
    style.Colors[ImGuiCol_Button] = oldColor;
    style.Colors[ImGuiCol_ButtonHovered] = oldHovered;
    style.Colors[ImGuiCol_ButtonActive] = oldActive;
    return clicked;
}