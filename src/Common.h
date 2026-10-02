#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wininet.h>        // ← 新增：INTERNET_MAX_URL_LENGTH
#include <dwmapi.h>
#include <commdlg.h>
#include <objbase.h>
#include <gdiplus.h>
#include <wincodec.h>
#include <wrl/client.h>     // ← 新增：Microsoft::WRL::ComPtr

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.Playback.h>

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

// 补齐 SDK 版本不一致时的 DWM 常量
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMSBT_TRANSIENTWINDOW
#define DWMSBT_TRANSIENTWINDOW 3
#endif

// 自定义消息
#define WM_TRAYICON        (WM_APP + 1)
#define WM_MEDIA_STATE     (WM_APP + 2)

namespace util {

inline std::wstring GetExeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring s(buf);
    auto pos = s.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? s : s.substr(0, pos + 1);
}

inline std::wstring AssetsPath(const std::wstring& file) {
    return GetExeDir() + L"assets\\" + file;
}

// 转成 file:/// 形式的 URL（处理空格、中文等）
inline std::wstring PathToFileUri(const std::wstring& path) {
    wchar_t buf[INTERNET_MAX_URL_LENGTH]{};
    DWORD len = INTERNET_MAX_URL_LENGTH;
    if (SUCCEEDED(UrlCreateFromPathW(path.c_str(), buf, &len, 0))) {
        return buf;
    }
    return {};
}

inline int S(int value, UINT dpi) {
    return MulDiv(value, dpi, 96);
}

// 打开文件对话框
inline std::wstring OpenAudioFileDialog(HWND owner) {
    wchar_t fileBuf[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = owner;
    ofn.lpstrFilter = L"音频文件\0*.mp3;*.wav;*.flac;*.m4a;*.aac;*.wma;*.ogg\0所有文件\0*.*\0";
    ofn.lpstrFile   = fileBuf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn)) return fileBuf;
    return {};
}

// 从 exe 的资源段读取指定名字的 RCDATA，返回字节向量
inline std::vector<uint8_t> LoadResourceBytes(const wchar_t* resName) {
    HINSTANCE hInst = GetModuleHandleW(nullptr);

    HRSRC hRes = FindResourceW(hInst, resName, RT_RCDATA);
    if (!hRes) return {};

    DWORD size = SizeofResource(hInst, hRes);
    if (size == 0) return {};

    HGLOBAL hGlobal = LoadResource(hInst, hRes);
    if (!hGlobal) return {};

    const void* data = LockResource(hGlobal);
    if (!data) return {};

    std::vector<uint8_t> buf(size);
    memcpy(buf.data(), data, size);
    return buf;
}

// 统一日志：追加写入 %TEMP%\MediaFlyout.log
inline void Log(const wchar_t* fmt, ...) {
    wchar_t path[MAX_PATH]{};
    GetTempPathW(MAX_PATH, path);
    wcscat_s(path, L"MediaFlyout.log");

    FILE* f = nullptr;
    _wfopen_s(&f, path, L"a, ccs=UTF-8");
    if (!f) return;

    SYSTEMTIME st{};
    GetLocalTime(&st);
    fwprintf(f, L"[%02d:%02d:%02d.%03d] ",
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list args;
    va_start(args, fmt);
    vfwprintf(f, fmt, args);
    va_end(args);

    fwprintf(f, L"\n");
    fclose(f);
}

} // namespace util