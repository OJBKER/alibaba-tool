#include "ImGuiMainWindow.h"
#include <algorithm>
#include "TopBar.h"
#include "MainContent.h"
#include "AutomaticPublishing.h"
#include "ViewProduct.h"
#include "Chat.h"
MainContent mainContent;
AutomaticPublishing automaticPublishing;
ViewProduct viewProduct;
Chat chat;
ImGuiMainWindow::ImGuiMainWindow(HWND hwnd)
    : hwnd_(hwnd), dragging_(false), bg_color_{1.0f, 1.0f, 1.0f, 1.0f} // 默认白色
{
    drag_start_mouse_.x = drag_start_mouse_.y = 0;
    drag_start_window_.left = drag_start_window_.top = drag_start_window_.right = drag_start_window_.bottom = 0;
}

void ImGuiMainWindow::SetBackgroundColor(const ImVec4& color)
{
    bg_color_ = color;
}

ImVec4 ImGuiMainWindow::GetBackgroundColor() const
{
    return bg_color_;
}

void ImGuiMainWindow::Render()
{
    static const int border_width = 8;
    static const int min_width = 300, min_height = 200;

    // 关键：去除主窗口的padding和border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // 设置窗口背景色
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color_);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::Begin("MainWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    bool closeRequested = false;
    ImVec2 win_size = ImGui::GetWindowSize();
    float topbar_height = 32.0f;

    // 只渲染当前可见的内容区
    mainContent.Render(topbar_height, win_size);
    automaticPublishing.Render(topbar_height, win_size);
    viewProduct.Render(topbar_height, win_size);
    chat.Render(topbar_height, win_size);

    // 再渲染TopBar，保证TopBar永远在最上层
    TopBar::Render(&closeRequested, topbar_height, win_size.x);

    if (closeRequested) {
        PostMessage(hwnd_, WM_CLOSE, 0, 0);
    }

    // 获取窗口和鼠标信息
    RECT rect;
    GetWindowRect(hwnd_, &rect);
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    ImVec2 local_mouse = ImVec2(mouse_pos.x - win_pos.x, mouse_pos.y - win_pos.y);

    // ==== 边缘缩放 ====
    static bool resizing = false;
    static int resize_edge = 0; // 1=left,2=right,4=top,8=bottom,可组合
    static POINT resize_start_mouse;
    static RECT resize_start_rect;

    // 检查鼠标是否在边缘
    int edge = 0;
    if (local_mouse.x >= 0 && local_mouse.x < border_width) edge |= 1; // left
    if (local_mouse.x >= win_size.x - border_width && local_mouse.x < win_size.x) edge |= 2; // right
    if (local_mouse.y >= 0 && local_mouse.y < border_width) edge |= 4; // top
    if (local_mouse.y >= win_size.y - border_width && local_mouse.y < win_size.y) edge |= 8; // bottom

    // 设置鼠标样式
    if (edge == (1 | 4) || edge == (2 | 8)) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
    else if (edge == (2 | 4) || edge == (1 | 8)) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
    else if (edge & (1 | 2)) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    else if (edge & (4 | 8)) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

    // 开始缩放
    if (!resizing && edge && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
        resizing = true;
        resize_edge = edge;
        GetCursorPos(&resize_start_mouse);
        GetWindowRect(hwnd_, &resize_start_rect);
    }
    // 缩放中
    if (resizing) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            POINT cur_mouse;
            GetCursorPos(&cur_mouse);
            int dx = cur_mouse.x - resize_start_mouse.x;
            int dy = cur_mouse.y - resize_start_mouse.y;
            RECT new_rect = resize_start_rect;
            if (resize_edge & 1) // left
                new_rect.left = std::min(resize_start_rect.right - min_width, resize_start_rect.left + dx);
            if (resize_edge & 2) // right
                new_rect.right = std::max(resize_start_rect.left + min_width, resize_start_rect.right + dx);
            if (resize_edge & 4) // top
                new_rect.top = std::min(resize_start_rect.bottom - min_height, resize_start_rect.top + dy);
            if (resize_edge & 8) // bottom
                new_rect.bottom = std::max(resize_start_rect.top + min_height, resize_start_rect.bottom + dy);

            // 保证窗口不会反向（left < right, top < bottom）
            if (new_rect.right - new_rect.left < min_width)
                new_rect.right = new_rect.left + min_width;
            if (new_rect.bottom - new_rect.top < min_height)
                new_rect.bottom = new_rect.top + min_height;

            MoveWindow(hwnd_, new_rect.left, new_rect.top, new_rect.right - new_rect.left, new_rect.bottom - new_rect.top, TRUE);
        } else {
            resizing = false;
        }
    }

    // 拖动窗口（禁止缩放时拖动）
    if (!dragging_ && !resizing) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            local_mouse.y >= 0 && local_mouse.y < 30 &&
            ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        {
            dragging_ = true;
            GetCursorPos(&drag_start_mouse_);
            GetWindowRect(hwnd_, &drag_start_window_);
        }
    } else if (dragging_) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            POINT cur_mouse;
            GetCursorPos(&cur_mouse);
            int dx = cur_mouse.x - drag_start_mouse_.x;
            int dy = cur_mouse.y - drag_start_mouse_.y;
            MoveWindow(hwnd_,
                drag_start_window_.left + dx,
                drag_start_window_.top + dy,
                drag_start_window_.right - drag_start_window_.left,
                drag_start_window_.bottom - drag_start_window_.top,
                TRUE);
        } else {
            dragging_ = false;
        }
    }

    // 绘制黑色描边
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImVec2 win_size2 = ImGui::GetWindowSize();
    float border_thickness = 1.0f;
    draw_list->AddRect(win_pos, ImVec2(win_pos.x + win_size2.x, win_pos.y + win_size2.y), IM_COL32(128,128,128,255), 0.0f, 0, border_thickness);

    ImGui::End();
    ImGui::PopStyleColor(); // ImGuiCol_WindowBg
    ImGui::PopStyleVar(2); // 恢复主窗口样式
}