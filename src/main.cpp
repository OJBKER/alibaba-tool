#include <windows.h>
#include "gui/ImGuiApp.h"
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <map>
#include "alibaba_api.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    SetConsoleOutputCP(65001);

    ImGuiApp app;
    if (!app.Init(hInstance))
        return 1;
    int ret = app.Run();
    app.Cleanup();
    return ret;
}

