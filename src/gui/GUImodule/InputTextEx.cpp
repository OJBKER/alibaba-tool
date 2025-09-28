#include "InputTextEx.h"
#include <imgui.h>
#include <string>

// 美化版InputText，支持label、hint、圆角、背景色、边框色
bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, const char* hint) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 origFrameBg = style.Colors[ImGuiCol_FrameBg];
    ImVec4 origBorder = style.Colors[ImGuiCol_Border];
    ImVec4 origText = style.Colors[ImGuiCol_Text];
    // 美化色彩
    ImVec4 frameBg = ImVec4(0.97f, 0.98f, 1.0f, 1.0f);
    ImVec4 border = ImVec4(0.60f, 0.80f, 1.0f, 1.0f);
    ImVec4 text = ImVec4(0.13f, 0.22f, 0.36f, 1.0f);
    float rounding = 6.0f;
    style.Colors[ImGuiCol_FrameBg] = frameBg;
    style.Colors[ImGuiCol_Border] = border;
    style.Colors[ImGuiCol_Text] = text;
    style.FrameRounding = rounding;
    bool changed = false;
    if (hint && buf[0] == '\0') {
        changed = ImGui::InputTextWithHint(label, hint, buf, buf_size, flags);
    } else {
        changed = ImGui::InputText(label, buf, buf_size, flags);
    }
    // 恢复
    style.Colors[ImGuiCol_FrameBg] = origFrameBg;
    style.Colors[ImGuiCol_Border] = origBorder;
    style.Colors[ImGuiCol_Text] = origText;
    style.FrameRounding = 0.0f;
    return changed;
}
