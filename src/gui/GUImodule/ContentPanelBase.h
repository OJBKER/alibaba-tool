#pragma once
#include <imgui.h>

class ContentPanelBase {
protected:
    bool visible_ = false;
public:
    virtual ~ContentPanelBase() = default;
    virtual const char* PanelName() const = 0;
    virtual void RenderContent() = 0;

    void Show();
    void Hide();
    bool IsVisible() const;

    void Render(float topbar_height, const ImVec2& win_size);
};