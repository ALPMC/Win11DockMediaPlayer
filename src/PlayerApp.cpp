#include "Common.h"
#include "PlayerApp.h"
#define Log util::Log
#include <cstdio>
#include <cstdarg>


// ===========================================================================
//  Initialize
// ===========================================================================
bool PlayerApp::Initialize(HINSTANCE hInstance) {
    Log(L"[App] ===== Initialize 开始 =====");
    m_hInstance = hInstance;

    // ---- 播放引擎 ----
    Log(L"[App] 创建 MediaEngine...");
    try {
        m_engine = std::make_unique<MediaEngine>();
        Log(L"[App] MediaEngine OK");
    } catch (const winrt::hresult_error& e) {
        Log(L"[App] MediaEngine 异常: 0x%08X %ls",
            (unsigned)e.code(), e.message().c_str());
        return false;
    }

    // ---- 弹出面板（先创建，因为 TrayIcon 要用它的 HWND）----
    Log(L"[App] 创建 FlyoutWindow...");
    m_flyout = std::make_unique<FlyoutWindow>();
    if (!m_flyout->Create(hInstance, m_engine.get())) {
        Log(L"[App] FlyoutWindow::Create 失败");
        return false;
    }
    Log(L"[App] FlyoutWindow OK, hwnd=%p", m_flyout->GetHwnd());

    // ---- 任务栏图标（复用 FlyoutWindow 的 HWND）----
    Log(L"[App] 创建 TrayIcon...");
    m_tray = std::make_unique<TrayIcon>();
    m_tray->OnLeftClick  = [this] { OnTrayLeftClick(); };
    m_tray->OnRightClick = [this] { OnTrayRightClick(); };

    // ★ 把托盘消息从 FlyoutWindow 转发到 TrayIcon
    m_flyout->OnTrayMessage = [this](WPARAM wp, LPARAM lp) {
        m_tray->HandleTrayMessage(wp, lp);
    };

    if (!m_tray->Create(m_flyout->GetHwnd(), L"PLAYER_PNG", L"音乐播放器")) {
        Log(L"[App] TrayIcon::Create 失败（具体错误见上面 [Tray] 日志）");
        return false;
    }
    Log(L"[App] TrayIcon 创建成功");
    Log(L"[App] ===== Initialize 完成 =====");
    return true;
}

// ===========================================================================
//  Run
// ===========================================================================
int PlayerApp::Run() {
    Log(L"[App] 进入消息循环");

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Log(L"[App] 消息循环结束, wParam=%zu", (size_t)msg.wParam);
    return (int)msg.wParam;
}

// ===========================================================================
//  OnTrayLeftClick
// ===========================================================================
void PlayerApp::OnTrayLeftClick() {
    Log(L"[App] 托盘左键点击");

    if (!m_tray || !m_flyout) return;

    RECT r = m_tray->GetIconRect();
    Log(L"[App] 图标矩形: (%ld, %ld) - (%ld, %ld)",
        r.left, r.top, r.right, r.bottom);

    m_flyout->ToggleNearTray(r);
    Log(L"[App] 面板可见性: %d", m_flyout->IsVisible() ? 1 : 0);
}

// ===========================================================================
//  OnTrayRightClick
// ===========================================================================
void PlayerApp::OnTrayRightClick() {
    Log(L"[App] 托盘右键点击");

    if (!m_tray || !m_flyout) return;

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, L"打开面板");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 2, L"退出");

    POINT pt{};
    GetCursorPos(&pt);
    HWND hwnd = m_tray->GetHwnd();
    SetForegroundWindow(hwnd);

    int cmd = TrackPopupMenu(menu,
                             TPM_RETURNCMD | TPM_RIGHTBUTTON,
                             pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);

    Log(L"[App] 菜单命令: %d", cmd);

    if (cmd == 1) {
        RECT r = m_tray->GetIconRect();
        m_flyout->ShowNearTray(r);
    } else if (cmd == 2) {
        PostQuitMessage(0);
    }
}