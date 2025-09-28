#pragma once
#include <imgui.h>
#include <string>

// 美化版InputText，支持label、hint、圆角、背景色、边框色
bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, const char* hint = nullptr);
