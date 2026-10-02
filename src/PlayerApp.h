#pragma once

#include "Common.h"

#include "TrayIcon.h"
#include "FlyoutWindow.h"
#include "MediaEngine.h"

class PlayerApp {
public:
    bool Initialize(HINSTANCE hInstance);
    int  Run();

private:
    void OnTrayLeftClick();
    void OnTrayRightClick();

    HINSTANCE m_hInstance = nullptr;
    std::unique_ptr<TrayIcon>     m_tray;
    std::unique_ptr<FlyoutWindow> m_flyout;
    std::unique_ptr<MediaEngine>  m_engine;
};