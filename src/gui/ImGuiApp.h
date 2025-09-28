#pragma once
#include <windows.h>
#include <d3d11.h>
#include <memory>
#include <string>
#include <chrono>
#include "ImGuiMainWindow.h"

class ImGuiApp {
public:
    ImGuiApp();
    ~ImGuiApp();

    bool Init(HINSTANCE hInstance);
    int Run();
    void Cleanup();

    static LRESULT WINAPI WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd_;
    WNDCLASSEX wc_;
    std::unique_ptr<ImGuiMainWindow> mainWindow_;

    ID3D11Device *g_pd3dDevice;
    ID3D11DeviceContext *g_pd3dDeviceContext;
    IDXGISwapChain *g_pSwapChain;
    ID3D11RenderTargetView *g_mainRenderTargetView;

    void CreateRenderTarget();
    void CleanupRenderTarget();
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool dx11_inited_ = false;
    bool win32_inited_ = false;

    // 新增：token管理
    std::string access_token_;
    std::string refresh_token_;
    std::chrono::steady_clock::time_point last_refresh_time_ = std::chrono::steady_clock::now();
    void TryRefreshAccessToken();
};