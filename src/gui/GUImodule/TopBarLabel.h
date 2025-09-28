#pragma once
#include <imgui.h>
#include <functional>
#include <string>

class TopBarLabel {
public:
    // labelText: 显示的文字
    // x, y, width, height: 位置和尺寸
    // onClick: 点击回调
    void Render(const std::string& labelText, float x, float y, float width, float height, std::function<void()> onClick = nullptr);
};