#include "PageTurner.h"
#include <imgui.h>
#include <functional>
#include <cstdlib>
#include <cstdio>

// 分页翻页控件，回调参数为新页码字符串
void PageTurner(const char* current_page, std::function<void(const char*)> onPageChange) {
    int page = atoi(current_page);
    ImGui::SameLine();
    if (ImGui::Button("上一页")) {
        if (page > 1) {
            char new_page[16];
            snprintf(new_page, sizeof(new_page), "%d", page - 1);
            onPageChange(new_page);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("下一页")) {
        char new_page[16];
        snprintf(new_page, sizeof(new_page), "%d", page + 1);
        onPageChange(new_page);
    }
}
