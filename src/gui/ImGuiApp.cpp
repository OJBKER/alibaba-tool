#include "ImGuiApp.h"
#include <tchar.h>
#include <iostream>
#include <imgui.h>
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "alibaba_api.h"
#include "helpers.h"
#include "GUImodule/my_popup.h"
#include <dwmapi.h>
#include <thread>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

ImGuiApp::ImGuiApp()
    : hwnd_(nullptr), wc_{}, g_pd3dDevice(nullptr), g_pd3dDeviceContext(nullptr),
      g_pSwapChain(nullptr), g_mainRenderTargetView(nullptr), dx11_inited_(false), win32_inited_(false) {}

ImGuiApp::~ImGuiApp() {
    Cleanup();
}

bool ImGuiApp::Init(HINSTANCE hInstance) {
    wc_ = {sizeof(WNDCLASSEX), CS_CLASSDC, ImGuiApp::WndProcStatic, 0L, 0L, hInstance, NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL};
    RegisterClassEx(&wc_);
    hwnd_ = CreateWindow(
        wc_.lpszClassName,
        _T("AlibabaTool ImGui Demo"),
        WS_POPUP | WS_VISIBLE, // ← 这里改为 WS_OVERLAPPEDWINDOW
        100, 100, 1280, 720,
        NULL, NULL, wc_.hInstance, this);

    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd_, &margins);

    if (!CreateDeviceD3D()) {
        CleanupDeviceD3D();
        UnregisterClass(wc_.lpszClassName, wc_.hInstance);
        return false;
    }

    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.MergeMode = false;
    font_cfg.OversampleH = font_cfg.OversampleV = 1;
    font_cfg.PixelSnapH = true;

    // 加载主字体（微软雅黑，支持中文）
    ImFont* font_main = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/msyh.ttc", 18.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull()
    );

    // 合并 emoji 字体
    font_cfg.MergeMode = true;
    static const ImWchar emoji_ranges[] = {
        static_cast<ImWchar>(0x1F300), static_cast<ImWchar>(0x1F64F),
        static_cast<ImWchar>(0x1F900), static_cast<ImWchar>(0x1F9FF),
        0
    };
    io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/seguiemj.ttf", 18.0f, &font_cfg, emoji_ranges
    );

    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1);
    win32_inited_ = ImGui_ImplWin32_Init(hwnd_);
    dx11_inited_ = ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // 初始化主窗口对象
    if (!mainWindow_) {
        mainWindow_ = std::make_unique<ImGuiMainWindow>(hwnd_);
    }

    return true;
}

void ImGuiApp::TryRefreshAccessToken() {
    // 只有last_response_map有内容时才尝试刷新token
    if (AlibabaAPI::last_response_map.empty()) return;
    std::string refresh_token = AlibabaAPI::last_response_map.count("refresh_token") ? AlibabaAPI::last_response_map["refresh_token"] : "";
    std::string access_token = AlibabaAPI::last_response_map.count("access_token") ? AlibabaAPI::last_response_map["access_token"] : "";
    if (access_token.empty() || refresh_token.empty()) return;
    //std::cout << "[刷新前refresh_token] " << refresh_token << std::endl;
    auto now = std::chrono::steady_clock::now();
    //五秒刷新
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_refresh_time_).count() < 3600) return;
    last_refresh_time_ = now;
    std::string new_token, err;
    std::string app_key = AlibabaAPI::config_map.count("app_key") ? AlibabaAPI::config_map["app_key"] : "";
    std::string app_secret = AlibabaAPI::config_map.count("app_secret") ? AlibabaAPI::config_map["app_secret"] : "";
    std::map<std::string, std::string> new_response_map;
    bool ok = AlibabaAPI::RefreshAccessToken(
        app_key, app_secret, refresh_token, new_token, err, &new_response_map
    );
    if (ok && !new_token.empty()) {
        AlibabaAPI::last_response_map = new_response_map; // 全量覆盖，确保refresh_token等都更新
        for (const auto& kv : AlibabaAPI::last_response_map) {
            //std::cout << "[刷新后] " << kv.first << ": " << kv.second << std::endl;
        }
    } else if (!err.empty()) {
        std::cout << "[Token刷新失败] " << err << std::endl;
    }
    // 可选：弹窗或日志提示
}

int ImGuiApp::Run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        // 每帧尝试刷新token
        TryRefreshAccessToken();
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        mainWindow_->Render();
        MyPopup::RenderPopup();

        ImGui::Render();
        ImVec4 bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        const float clear_color_with_alpha[4] = {bg.x, bg.y, bg.z, 1.0f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // Vsync
        // 可选：降低CPU占用
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}

void ImGuiApp::Cleanup() {
    // 清理
    if (dx11_inited_) {
        ImGui_ImplDX11_Shutdown();
        dx11_inited_ = false;
    }
    if (win32_inited_) {
        ImGui_ImplWin32_Shutdown();
        win32_inited_ = false;
    }
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    if (wc_.lpszClassName && wc_.hInstance)
        UnregisterClass(wc_.lpszClassName, wc_.hInstance);
}

LRESULT WINAPI ImGuiApp::WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGuiApp* app = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<ImGuiApp*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)app);
    } else {
        app = reinterpret_cast<ImGuiApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    if (app) return app->WndProc(hWnd, msg, wParam, lParam);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT ImGuiApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ImGuiApp::CreateRenderTarget() {
    ID3D11Texture2D *pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
void ImGuiApp::CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}
bool ImGuiApp::CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 1280;
    sd.BufferDesc.Height = 720;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd_;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = {D3D_FEATURE_LEVEL_11_0};
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 1,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}
void ImGuiApp::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}