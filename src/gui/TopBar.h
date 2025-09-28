#pragma once
#include <imgui.h>
#include <string>

class TopBar {
public:
    // text: 左侧文本，onClose: 关闭按钮被点击时回调
    // 增加 width 参数
    static void Render(bool* closeRequested = nullptr, float height = 32.0f, float width = 0.0f);
};