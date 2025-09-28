#pragma once
#include "GUImodule/ContentPanelBase.h"

class MainContent : public ContentPanelBase {
public:
    const char* PanelName() const override;
    void RenderContent() override;
};