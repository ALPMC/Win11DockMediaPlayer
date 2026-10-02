cmake_minimum_required(VERSION 3.20)
project(MediaFlyout LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "Windows only")
endif()

# 定位 Windows SDK
if(DEFINED ENV{WindowsSdkDir})
    set(WINSDK_DIR "$ENV{WindowsSdkDir}")
else()
    set(WINSDK_DIR "C:/Program Files (x86)/Windows Kits/10")
endif()

file(GLOB WINSDK_VERSIONS "${WINSDK_DIR}/Include/10.*")
list(SORT WINSDK_VERSIONS)
list(REVERSE WINSDK_VERSIONS)
list(GET WINSDK_VERSIONS 0 WINSDK_INCLUDE_ROOT)
get_filename_component(WINSDK_VERSION "${WINSDK_INCLUDE_ROOT}" NAME)

message(STATUS "Windows SDK: ${WINSDK_VERSION}")

set(WINSDK_INC "${WINSDK_DIR}/Include/${WINSDK_VERSION}")
set(WINSDK_LIB "${WINSDK_DIR}/Lib/${WINSDK_VERSION}")

add_executable(MediaFlyout WIN32
    src/main.cpp
    src/PlayerApp.cpp
    src/TrayIcon.cpp
    src/MediaEngine.cpp
    src/FlyoutWindow.cpp
    resources/app.rc
)

target_include_directories(MediaFlyout PRIVATE
    src
    "${WINSDK_INC}/cppwinrt"
    "${WINSDK_INC}/um"
    "${WINSDK_INC}/shared"
    "${WINSDK_INC}/winrt"
)

target_compile_definitions(MediaFlyout PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00
)

target_compile_options(MediaFlyout PRIVATE
    /utf-8 /EHsc /W3 /permissive- /Zc:__cplusplus
)

target_link_directories(MediaFlyout PRIVATE
    "${WINSDK_LIB}/um/x64"
    "${WINSDK_LIB}/ucrt/x64"
)

target_link_libraries(MediaFlyout PRIVATE
    windowsapp       # WinRT (含 Windows.Media.Playback)
    gdiplus
    windowscodecs
    dwmapi
    shell32 user32 gdi32 ole32 shlwapi comdlg32
)

target_link_options(MediaFlyout PRIVATE /MANIFEST:NO)
