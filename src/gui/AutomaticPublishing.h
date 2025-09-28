#pragma once
#include "GUImodule/ContentPanelBase.h"
#include <string>
void GetProductSchemaByCatId(const std::string& catid, const char* language);

class AutomaticPublishing : public ContentPanelBase {
public:
    const char* PanelName() const override;
    void RenderContent() override;
};