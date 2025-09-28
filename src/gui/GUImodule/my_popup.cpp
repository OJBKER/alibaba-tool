#include "my_popup.h"
#include <string>

namespace {
    static std::string g_popup_title;
    static std::string g_popup_message;
    static bool g_popup_open = false;
}

namespace MyPopup {

void ShowPopupButton() {
    if (ImGui::Button("弹出窗口")) {
        ImGui::OpenPopup("我的弹窗");
    }
}

void Show(const char* title, const char* message) {
    g_popup_title = title;
    g_popup_message = message;
    g_popup_open = true;
    ImGui::OpenPopup(g_popup_title.c_str());
}

void RenderPopup() {
    if (g_popup_open) {
        // 设置弹窗背景为白色
        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(255,255,255,255));
        if (ImGui::BeginPopupModal(g_popup_title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0,0,0,1), "%s", g_popup_message.c_str()); // 文字黑色
            if (ImGui::Button("关闭")) {
                ImGui::CloseCurrentPopup();
                g_popup_open = false;
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
    }
}

}