#pragma once
#include "DropdownMenuStyle.h"
#include <functional>

class MyDropdownMenu : public DropdownMenuStyle {
public:
    void Render(const char* popup_id, std::function<void(int)> onMenuClick);
};