#include "helpers.h"
#include <imgui.h>
#include <filesystem>
#include "../gui/GUImodule/my_popup.h"

bool LoadChineseFont(const char* font_path, float font_size)
{
    namespace fs = std::filesystem;
    ImGuiIO& io = ImGui::GetIO();
    if (fs::exists(font_path))
    {
        io.Fonts->AddFontFromFileTTF(font_path, font_size, NULL, io.Fonts->GetGlyphRangesChineseFull());
        return true;
    }
    // 加载失败弹出窗口
    MyPopup::Show("Font loading failed", "Chinese font file not found. Chinese characters may not display properly!");
    return false;
}