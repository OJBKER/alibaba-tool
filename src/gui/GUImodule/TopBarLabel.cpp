#include "TopBarLabel.h"

void TopBarLabel::Render(const std::string& labelText, float x, float y, float width, float height, std::function<void()> onClick) {
    ImGui::SetCursorPos(ImVec2(x, y));

    // 透明按钮颜色，悬停半透明白，点击半透明黑
    ImVec4 normal = ImVec4(0, 0, 0, 0);
    ImVec4 hovered = ImVec4(1, 1, 1, 0.5f);
    ImVec4 active  = ImVec4(0, 0, 0, 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);

    if (ImGui::Button(labelText.c_str(), ImVec2(width, height))) {
        if (onClick) onClick();
    }

    ImGui::PopStyleColor(3);
}