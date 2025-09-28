#include "TopBar.h"
#include "GUImodule/CloseButton.h"
#include "GUImodule/TopBarMenu.h"
#include "MainContent.h"
#include "AutomaticPublishing.h"
#include "ViewProduct.h"
#include "GUImodule/TopBarLabel.h"
#include "Chat.h"

extern MainContent mainContent;
extern AutomaticPublishing automaticPublishing;
extern ViewProduct viewProduct;
extern Chat chat;

void TopBar::Render(bool* closeRequested, float height, float width)
{
    if (width <= 0.0f)
        width = ImGui::GetWindowWidth();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.92f, 0.93f, 0.96f, 1.0f));
    ImGui::BeginChild("##TopBar", ImVec2(width, height), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    float logo_width = 40.0f;
    float close_width = 50.0f;
    float spacing = 10.0f;

    // 控件总宽度为bar的15%
    float controls_total_width = width * 0.15f;
    float label_width = controls_total_width * 0.5f;
    float menu_width = controls_total_width * 0.5f;

    // 左侧LOGO
    ImGui::SetCursorPos(ImVec2(10, (height - ImGui::GetFontSize()) * 0.5f));
    ImGui::TextUnformatted("[*]");

    // 首页标签（紧跟LOGO右侧）
    float label_x = logo_width + spacing;
    static TopBarLabel homeLabel;
    homeLabel.Render("首页", label_x, 0, label_width, height, [](){
        mainContent.Show();
        automaticPublishing.Hide();
        viewProduct.Hide();
    });

    // 菜单按钮（紧跟首页标签右侧）
    float menu_x = label_x + label_width;
    static TopBarMenu menu;
    menu.Render(menu_x, 0, menu_width, height, [](int idx){
        mainContent.Hide();
        automaticPublishing.Hide();
        viewProduct.Hide();
        chat.Hide();
        if (idx == 1) automaticPublishing.Show();
        if (idx == 2) viewProduct.Show();
        if (idx == 3) mainContent.Show();
        if (idx == 4) chat.Show(); // 新增菜单选项4对应Chat页面
    });

    // 右侧关闭按钮
    ImGui::SetCursorPos(ImVec2(width - close_width, 0));
    if (CloseButton::Render(0.0f, height)) {
        if (closeRequested) *closeRequested = true;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}