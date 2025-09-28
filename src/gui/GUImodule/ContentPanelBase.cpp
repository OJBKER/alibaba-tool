#include "ContentPanelBase.h"

void ContentPanelBase::Show() { visible_ = true; }
void ContentPanelBase::Hide() { visible_ = false; }
bool ContentPanelBase::IsVisible() const { return visible_; }

void ContentPanelBase::Render(float topbar_height, const ImVec2& win_size) {
    if (!visible_) return;

    const float border_width = 8.0f;
    ImVec2 content_pos(border_width, topbar_height);
    ImVec2 content_size(win_size.x - border_width * 2, win_size.y - topbar_height - border_width);

    ImGui::SetNextWindowPos(content_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(content_size, ImGuiCond_Always);

    ImGui::Begin(PanelName(), nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings);

    RenderContent();

    ImGui::End();
}