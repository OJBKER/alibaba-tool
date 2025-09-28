cmake_minimum_required(VERSION 3.10)

project(AlibabaTool)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 定义源文件路径
file(GLOB SOURCES
    src/*.cpp
    src/gui/*.cpp
    src/gui/GUImodule/*.cpp
    src/api/*.cpp
    src/utils/*.cpp
)

# 定义 ImGui 目录
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/src/gui/imgui")

# 添加 ImGui 源文件
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp      
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

# 添加可执行目标
add_executable(AlibabaTool ${SOURCES} ${IMGUI_SOURCES})

add_custom_command(TARGET AlibabaTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/src/cacert.pem
            $<TARGET_FILE_DIR:AlibabaTool>/cacert.pem
)

# 包含头文件路径
target_include_directories(AlibabaTool PRIVATE 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
    ${PROJECT_SOURCE_DIR}/include
    "C:/msys64/mingw64/include"
)

# 链接 DirectX 库（如不需要可注释）
target_link_libraries(AlibabaTool PRIVATE
    d3d11 dxgi dxguid d3dcompiler dwmapi
    "C:/msys64/mingw64/lib/libcurl.dll.a"
)

find_package(OpenSSL REQUIRED)
target_link_libraries(AlibabaTool PRIVATE OpenSSL::Crypto)
target_link_libraries(AlibabaTool PRIVATE tinyxml2)

# 可执行文件输出目录
set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
if(NOT OUTPUT_DIR)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif()


# 需要复制的 DLL 列表
set(MSYS2_DLLS
    "C:/msys64/mingw64/bin/libstdc++-6.dll"
    "C:/msys64/mingw64/bin/libgcc_s_seh-1.dll"
    "C:/msys64/mingw64/bin/libwinpthread-1.dll"
    "C:/msys64/mingw64/bin/libcurl-4.dll"
)

foreach(dll ${MSYS2_DLLS})
    add_custom_command(TARGET AlibabaTool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${dll}" "$<TARGET_FILE_DIR:AlibabaTool>"
    )
endforeach()

add_definitions(-DUNICODE -D_UNICODE)