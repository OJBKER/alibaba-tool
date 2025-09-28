#pragma once
#include <imgui.h>
#include <functional>

class TopBarMenu {
public:
    // 可传入回调处理菜单项点击
    void Render(float x, float y, float width, float height, std::function<void(int)> onMenuClick = nullptr);
};