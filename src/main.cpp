#include "Common.h"
#include "PlayerApp.h"

#include <gdiplus.h>
#include <cstdio>
#include <cstdarg>

// ---------------------------------------------------------------------------
// 写日志到 %TEMP%\MediaFlyout.log
// ---------------------------------------------------------------------------
static void LogToFile(const wchar_t* fmt, ...) {
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE,
                      _In_ LPWSTR,
                      _In_ int)
{
    LogToFile(L"--- wWinMain 进入 ---");

    // WinRT 初始化
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        LogToFile(L"winrt::init_apartment OK");
    } catch (const std::exception& e) {
        LogToFile(L"winrt::init_apartment 失败: %hs", e.what());
        return -1;
    } catch (...) {
        LogToFile(L"winrt::init_apartment 未知异常");
        return -1;
    }

    // GDI+ 初始化
    Gdiplus::GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken = 0;
    auto gdiStatus = Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, nullptr);
    LogToFile(L"GdiplusStartup status=%d", (int)gdiStatus);

    int result = 0;
    try {
        PlayerApp app;
        LogToFile(L"PlayerApp 构造完成");

        if (app.Initialize(hInstance)) {
            LogToFile(L"Initialize OK，进入消息循环");
            result = app.Run();
            LogToFile(L"消息循环退出，result=%d", result);
        } else {
            LogToFile(L"Initialize 返回 false，程序将退出");
            MessageBoxW(nullptr,
                        L"初始化失败，详见 %TEMP%\\MediaFlyout.log",
                        L"MediaFlyout", MB_ICONERROR);
        }
    } catch (const winrt::hresult_error& e) {
        LogToFile(L"HRESULT 异常: 0x%08X %ls",
                  (unsigned)e.code(), e.message().c_str());
        MessageBoxW(nullptr, e.message().c_str(), L"HRESULT 异常", MB_ICONERROR);
    } catch (const std::exception& e) {
        LogToFile(L"std::exception: %hs", e.what());
        MessageBoxW(nullptr, L"发生标准异常，详见日志", L"错误", MB_ICONERROR);
    } catch (...) {
        LogToFile(L"未知异常");
        MessageBoxW(nullptr, L"未知异常", L"错误", MB_ICONERROR);
    }

    Gdiplus::GdiplusShutdown(gdiToken);
    LogToFile(L"--- wWinMain 退出 ---");
    return result;
}