#include "MyDropdownMenu.h"
#include <imgui.h>
#include <iostream>

void MyDropdownMenu::Render(const char* popup_id, std::function<void(int)> onMenuClick) {
    // 使用基类样式
    PushStyle();
    if (ImGui::BeginPopup(popup_id)) {
        if (ImGui::MenuItem("选项1")) {
            if (onMenuClick) onMenuClick(1);
            else std::cout << "选项1" << std::endl;
        }
        if (ImGui::MenuItem("选项2")) {
            if (onMenuClick) onMenuClick(2);
            else std::cout << "选项2" << std::endl;
        }
        if (ImGui::MenuItem("选项3")) {
            if (onMenuClick) onMenuClick(3);
            else std::cout << "选项3" << std::endl;
        }
        // 新增选项4
        if (ImGui::MenuItem("选项4")) {
            if (onMenuClick) onMenuClick(4);
            else std::cout << "选项4" << std::endl;
        }
        ImGui::EndPopup();
    }
    PopStyle();
}