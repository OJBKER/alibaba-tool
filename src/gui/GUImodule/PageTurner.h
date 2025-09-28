#pragma once
#include <functional>

// 分页翻页控件，回调参数为新页码字符串
void PageTurner(const char* current_page, std::function<void(const char*)> onPageChange);
