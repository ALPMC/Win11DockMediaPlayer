#pragma once

#include "Common.h"

class MediaEngine;

class FlyoutWindow {
public:
    FlyoutWindow();
    ~FlyoutWindow();

    bool Create(HINSTANCE hInstance, MediaEngine* engine);

    void ShowNearTray(const RECT& trayRect);
    void Hide();
    void ToggleNearTray(const RECT& trayRect);
    bool IsVisible() const { return m_visible; }
    void Refresh();

    HWND GetHwnd() const { return m_hwnd; }
    std::function<void(WPARAM, LPARAM)> OnTrayMessage;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);

    void Render();
    void DrawBackground(Gdiplus::Graphics& g);
    void DrawTitle(Gdiplus::Graphics& g);
    void DrawPlayButton(Gdiplus::Graphics& g);
    void DrawOpenButton(Gdiplus::Graphics& g);
    void DrawLoopButton(Gdiplus::Graphics& g);        // ★ 新增

    void OnClick(int x, int y);
    bool LoadImages();
    void Layout();
    UINT Dpi() const;

    HWND   m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    MediaEngine* m_engine = nullptr;

    static constexpr int kBaseW = 320;      // ★ 稍宽一点，容纳循环按钮
    static constexpr int kBaseH = 210;

    int m_width = 0;
    int m_height = 0;

    RECT m_playBtnRect{};
    RECT m_openBtnRect{};
    RECT m_loopBtnRect{};                   // ★ 新增

    Gdiplus::Image* m_imgPlay  = nullptr;
    Gdiplus::Image* m_imgPause = nullptr;
    Gdiplus::Image* m_imgLoop  = nullptr;   // ★ 新增

    std::vector<uint8_t> m_playBytes;
    std::vector<uint8_t> m_pauseBytes;
    std::vector<uint8_t> m_loopBytes;       // ★ 新增

    bool m_visible = false;
    bool m_hoverPlay = false;
    bool m_hoverOpen = false;
    bool m_hoverLoop = false;               // ★ 新增
    bool m_trackingMouse = false;

    static constexpr UINT_PTR kTimerHideOnBlur = 1;
    static constexpr UINT_PTR kTimerHover      = 2;
};