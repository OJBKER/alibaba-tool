#pragma once
#include <imgui.h>

class DropdownMenuStyle {
public:
    virtual ~DropdownMenuStyle();
    virtual void PushStyle() const;
    virtual void PopStyle() const;
};