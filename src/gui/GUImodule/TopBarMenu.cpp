#include "TopBarMenu.h"
#include "MyDropdownMenu.h"
#include <imgui.h>
#include <iostream>

void TopBarMenu::Render(float x, float y, float width, float height, std::function<void(int)> onMenuClick) {
    ImGui::SetCursorPos(ImVec2(x, y));

    ImVec4 normal = ImVec4(0, 0, 0, 0);
    ImVec4 hovered = ImVec4(1, 1, 1, 0.5f);
    ImVec4 active  = ImVec4(0.7f, 0.7f, 0.7f, 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);

    if (ImGui::Button("菜单", ImVec2(width, height))) {
        ImGui::OpenPopup("MyDropdownMenu");
    }

    ImGui::PopStyleColor(3);

    // 使用继承自DropdownMenuStyle的MyDropdownMenu对象
    static MyDropdownMenu menuStyle;
    menuStyle.Render("MyDropdownMenu", onMenuClick);
}