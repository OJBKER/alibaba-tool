#pragma once
#include "GUImodule/ContentPanelBase.h"

class Chat : public ContentPanelBase {
public:
    virtual const char* PanelName() const override { return "Chat"; }
    virtual void RenderContent() override;
};