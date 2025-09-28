#include "DropdownMenuStyle.h"

DropdownMenuStyle::~DropdownMenuStyle() = default;

void DropdownMenuStyle::PushStyle() const {
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1, 1, 1, 1)); // 默认白色背景
}

void DropdownMenuStyle::PopStyle() const {
    ImGui::PopStyleColor();
}