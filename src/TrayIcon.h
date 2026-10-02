#pragma once

#include "Common.h"

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();

    // ★ 新签名：接收外部 HWND，不再自己创建窗口
    bool Create(HWND hwnd, const wchar_t* resName,
                const std::wstring& tooltip);
    void Destroy();

    HWND GetHwnd() const { return m_hwnd; }
    RECT GetIconRect() const;

    // ★ 由 FlyoutWindow 转发 WM_TRAYICON_ 消息进来
    void HandleTrayMessage(WPARAM wp, LPARAM lp);

    std::function<void()> OnLeftClick;
    std::function<void()> OnRightClick;

    // 供 FlyoutWindow 在 WndProc 里判断是否是托盘消息
    static constexpr UINT WM_TRAYICON_ = WM_APP + 100;

private:
    static HICON LoadHIconFromResource(const wchar_t* resName, int size);

    HWND            m_hwnd = nullptr;
    HICON           m_hIcon = nullptr;
    NOTIFYICONDATAW m_nid{};
    bool            m_registered = false;
};