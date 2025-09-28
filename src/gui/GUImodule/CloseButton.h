#pragma once
#include <imgui.h>

class CloseButton {
public:
    // 返回true表示被点击
    // height: 按钮高度（与菜单栏一致），宽度恒为50像素
    static bool Render(float offset_x = 0.0f, float height = 32.0f);
};