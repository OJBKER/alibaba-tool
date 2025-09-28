#pragma once
#include <windows.h>
#include <imgui.h>

class ImGuiMainWindow {
public:
    ImGuiMainWindow(HWND hwnd);
    void Render();
    void SetBackgroundColor(const ImVec4& color);
    ImVec4 GetBackgroundColor() const;

private:
    HWND hwnd_;
    bool dragging_;
    POINT drag_start_mouse_;
    RECT drag_start_window_;
    ImVec4 bg_color_;
};