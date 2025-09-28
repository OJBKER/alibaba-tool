#pragma once
#include <imgui.h>
#include <string>

namespace MyPopup {
    void ShowPopupButton();
    void RenderPopup();
    void Show(const char* title, const char* message);
}