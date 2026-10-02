#include "Common.h"
#include "FlyoutWindow.h"
#include "MediaEngine.h"

using namespace Gdiplus;

static const wchar_t* kFlyoutClass = L"MediaFlyoutPanelWnd";

FlyoutWindow::FlyoutWindow() = default;

FlyoutWindow::~FlyoutWindow() {
    if (m_hwnd) DestroyWindow(m_hwnd);
    delete m_imgPlay;
    delete m_imgPause;
    delete m_imgLoop;
}

UINT FlyoutWindow::Dpi() const {
    return m_hwnd ? GetDpiForWindow(m_hwnd) : 96;
}

// ---------------------------------------------------------------------------
bool FlyoutWindow::LoadImages() {
    // 播放
    {
        m_playBytes = util::LoadResourceBytes(L"PLAY_PNG");
        if (!m_playBytes.empty()) {
            auto stream = SHCreateMemStream(m_playBytes.data(),
                                            (UINT)m_playBytes.size());
            if (stream) {
                m_imgPlay = Image::FromStream(stream);
                stream->Release();
            }
        }
    }
    // 暂停
    {
        m_pauseBytes = util::LoadResourceBytes(L"PAUSE_PNG");
        if (!m_pauseBytes.empty()) {
            auto stream = SHCreateMemStream(m_pauseBytes.data(),
                                            (UINT)m_pauseBytes.size());
            if (stream) {
                m_imgPause = Image::FromStream(stream);
                stream->Release();
            }
        }
    }
    // 循环
    {
        m_loopBytes = util::LoadResourceBytes(L"LOOP_PNG");
        if (!m_loopBytes.empty()) {
            auto stream = SHCreateMemStream(m_loopBytes.data(),
                                            (UINT)m_loopBytes.size());
            if (stream) {
                m_imgLoop = Image::FromStream(stream);
                stream->Release();
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
bool FlyoutWindow::Create(HINSTANCE hInstance, MediaEngine* engine) {
    m_hInstance = hInstance;
    m_engine    = engine;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_DROPSHADOW;
    wc.lpfnWndProc   = &FlyoutWindow::WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kFlyoutClass;

    if (!RegisterClassExW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    m_hwnd = CreateWindowExW(exStyle, kFlyoutClass, L"",
                             WS_POPUP,
                             0, 0, 100, 100,
                             nullptr, nullptr, hInstance, this);
    if (!m_hwnd) return false;

    LoadImages();
    Layout();

    if (m_engine) m_engine->SetNotifyWindow(m_hwnd);
    return true;
}

// ---------------------------------------------------------------------------
void FlyoutWindow::Layout() {
    UINT dpi = Dpi();
    m_width  = util::S(kBaseW, dpi);
    m_height = util::S(kBaseH, dpi);

    // 播放/暂停按钮：直径 96，居中偏上
    int btnSize = util::S(96, dpi);
    int btnX    = (m_width - btnSize) / 2;
    int btnY    = util::S(58, dpi);
    m_playBtnRect = { btnX, btnY, btnX + btnSize, btnY + btnSize };

    // 打开文件按钮：宽 120，高 32，底部居中偏左
    int ow = util::S(120, dpi);
    int oh = util::S(32, dpi);
    int ox = (m_width - ow) / 2 - util::S(24, dpi);
    int oy = m_height - util::S(20, dpi) - oh;
    m_openBtnRect = { ox, oy, ox + ow, oy + oh };

    // 循环按钮：32x32，放在打开按钮右侧，垂直居中
    int ls = util::S(32, dpi);
    int lx = ox + ow + util::S(12, dpi);
    int ly = oy + (oh - ls) / 2;
    m_loopBtnRect = { lx, ly, lx + ls, ly + ls };
}

// ---------------------------------------------------------------------------
LRESULT CALLBACK FlyoutWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    auto self = reinterpret_cast<FlyoutWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT FlyoutWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // 托盘消息转发
    if (msg == (WM_APP + 100)) {
        if (OnTrayMessage) OnTrayMessage(wp, lp);
        return 0;
    }

    switch (msg) {
    // 注意：这里删除了 WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    // 允许窗口正常激活，才能收到 WM_ACTIVATE 失活通知

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lp);
        int y = GET_Y_LPARAM(lp);

        // 兜底：即使窗口没获得前台焦点，SetCapture 也能让外部点击到这里。
        // 用客户区坐标判断是否落在面板矩形内。
        RECT rc;
        GetClientRect(hwnd, &rc);
        POINT pt{ x, y };
        if (!PtInRect(&rc, pt)) {
            Hide();
            return 0;
        }

        OnClick(x, y);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!m_trackingMouse) {
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            m_trackingMouse = true;
        }
        int x = GET_X_LPARAM(lp);
        int y = GET_Y_LPARAM(lp);
        POINT pt{ x, y };
        bool hp = PtInRect(&m_playBtnRect, pt);
        bool ho = PtInRect(&m_openBtnRect, pt);
        bool hl = PtInRect(&m_loopBtnRect, pt);
        if (hp != m_hoverPlay || ho != m_hoverOpen || hl != m_hoverLoop) {
            m_hoverPlay = hp;
            m_hoverOpen = ho;
            m_hoverLoop = hl;
            Render();
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        m_trackingMouse = false;
        m_hoverPlay = m_hoverOpen = m_hoverLoop = false;
        Render();
        return 0;

    case WM_MEDIA_STATE:
        Render();
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wp) == WA_INACTIVE) {
            // 失去前台焦点后延迟 250ms 隐藏。
            // 延迟的意义：用户点托盘图标"再次切换"时，面板先失活，
            // 紧接着收到 WM_TRAYICON 消息主动切换；没有延迟会两次隐藏/显示冲突闪烁。
            if (m_visible) {
                SetTimer(hwnd, kTimerHideOnBlur, 250, nullptr);
            }
        } else {
            // 重新获得焦点，取消待关闭
            KillTimer(hwnd, kTimerHideOnBlur);
        }
        return 0;

    case WM_TIMER:
        if (wp == kTimerHideOnBlur) {
            KillTimer(hwnd, kTimerHideOnBlur);
            if (m_visible && GetForegroundWindow() != hwnd) {
                Hide();
            }
        }
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { Hide(); return 0; }
        break;

    case WM_DESTROY:
        return 0;

    case WM_NCHITTEST:
        return HTCLIENT;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
void FlyoutWindow::Render() {
    if (!m_hwnd) return;

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = m_width;
    bmi.bmiHeader.biHeight      = -m_height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP memBmp = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    memset(bits, 0, (size_t)m_width * m_height * 4);

    {
        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        g.SetCompositingQuality(CompositingQualityHighQuality);

        DrawBackground(g);
        DrawTitle(g);
        DrawPlayButton(g);
        DrawOpenButton(g);
        DrawLoopButton(g);
    }

    RECT wr{};
    GetWindowRect(m_hwnd, &wr);

    POINT ptSrc{ 0, 0 };
    POINT ptDst{ wr.left, wr.top };
    SIZE  size{ m_width, m_height };
    BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(m_hwnd, screenDC, &ptDst, &size,
                        memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

// ---------------------------------------------------------------------------
void FlyoutWindow::DrawBackground(Graphics& g) {
    int r = util::S(8, Dpi());

    GraphicsPath path;
    path.AddArc(0, 0, r * 2, r * 2, 180, 90);
    path.AddArc(m_width - r * 2, 0, r * 2, r * 2, 270, 90);
    path.AddArc(m_width - r * 2, m_height - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(0, m_height - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();

    SolidBrush bg(Color(235, 32, 32, 32));
    g.FillPath(&bg, &path);

    Pen border(Color(40, 255, 255, 255), 1.0f);
    g.DrawPath(&border, &path);
}

void FlyoutWindow::DrawTitle(Graphics& g) {
    const wchar_t* text = L"未选择音乐";
    static std::wstring buf;
    if (m_engine && m_engine->HasMedia()) {
        const std::wstring& path = m_engine->CurrentFile();
        auto pos = path.find_last_of(L"\\/");
        buf = (pos == std::wstring::npos) ? path : path.substr(pos + 1);
        text = buf.c_str();
    }

    UINT dpi = Dpi();
    FontFamily ff(L"Microsoft YaHei UI");
    Font font(&ff, (REAL)util::S(13, dpi), FontStyleRegular, UnitPixel);
    SolidBrush brush(Color(255, 220, 220, 220));

    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap);

    RectF rc((REAL)util::S(40, dpi), (REAL)util::S(20, dpi),
             (REAL)(m_width - util::S(80, dpi)), (REAL)util::S(24, dpi));
    g.DrawString(text, -1, &font, rc, &fmt, &brush);
}

void FlyoutWindow::DrawPlayButton(Graphics& g) {
    if (!m_engine) return;

    bool playing = m_engine->IsPlaying();
    int cx = (m_playBtnRect.left + m_playBtnRect.right) / 2;
    int cy = (m_playBtnRect.top + m_playBtnRect.bottom) / 2;
    int d  = m_playBtnRect.right - m_playBtnRect.left;

    Color fillCol = m_hoverPlay ? Color(255, 90, 90, 90) : Color(255, 70, 70, 70);
    SolidBrush bgBrush(fillCol);
    g.FillEllipse(&bgBrush, (REAL)(cx - d/2), (REAL)(cy - d/2), (REAL)d, (REAL)d);

    Image* icon = playing ? m_imgPause : m_imgPlay;
    int iconSize = (int)(d * 0.42);
    int ix = cx - iconSize / 2;
    int iy = cy - iconSize / 2;

    if (icon && icon->GetLastStatus() == Ok) {
        g.DrawImage(icon, ix, iy, iconSize, iconSize);
    } else {
        SolidBrush wb(Color(255, 240, 240, 240));
        if (playing) {
            int bw = iconSize / 3;
            g.FillRectangle(&wb, ix, iy, bw, iconSize);
            g.FillRectangle(&wb, ix + bw * 2, iy, bw, iconSize);
        } else {
            Point pts[3] = {
                { ix,            iy },
                { ix + iconSize, iy + iconSize / 2 },
                { ix,            iy + iconSize }
            };
            g.FillPolygon(&wb, pts, 3);
        }
    }
}

void FlyoutWindow::DrawOpenButton(Graphics& g) {
    int x = m_openBtnRect.left;
    int y = m_openBtnRect.top;
    int w = m_openBtnRect.right  - m_openBtnRect.left;
    int h = m_openBtnRect.bottom - m_openBtnRect.top;
    int r = util::S(6, Dpi());

    GraphicsPath path;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();

    Color fillCol = m_hoverOpen ? Color(255, 90, 90, 90) : Color(255, 60, 60, 60);
    SolidBrush brush(fillCol);
    g.FillPath(&brush, &path);

    UINT dpi = Dpi();
    FontFamily ff(L"Microsoft YaHei UI");
    Font font(&ff, (REAL)util::S(12, dpi), FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(255, 235, 235, 235));

    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);

    RectF rc((REAL)x, (REAL)y, (REAL)w, (REAL)h);
    g.DrawString(L"打开文件…", -1, &font, rc, &fmt, &textBrush);
}

void FlyoutWindow::DrawLoopButton(Graphics& g) {
    if (!m_engine) return;

    bool looping = m_engine->IsLooping();

    int x = m_loopBtnRect.left;
    int y = m_loopBtnRect.top;
    int d = m_loopBtnRect.right - m_loopBtnRect.left;
    if (d <= 0) return;

    if (looping) {
        SolidBrush accent(Color(255, 0, 120, 212));
        g.FillEllipse(&accent, (REAL)x, (REAL)y, (REAL)d, (REAL)d);
    } else if (m_hoverLoop) {
        SolidBrush hover(Color(255, 80, 80, 80));
        g.FillEllipse(&hover, (REAL)x, (REAL)y, (REAL)d, (REAL)d);
    }

    if (m_imgLoop && m_imgLoop->GetLastStatus() == Ok) {
        int iconSize = (int)(d * 0.6);
        int ix = x + (d - iconSize) / 2;
        int iy = y + (d - iconSize) / 2;
        g.DrawImage(m_imgLoop, ix, iy, iconSize, iconSize);
    } else {
        Color c = looping ? Color(255, 255, 255, 255)
                          : Color(200, 220, 220, 220);
        Pen pen(c, 2.0f);
        g.DrawArc(&pen, (REAL)x + d*0.25f, (REAL)y + d*0.25f,
                  (REAL)d*0.5f, (REAL)d*0.5f, 45.0f, 270.0f);
    }
}

// ---------------------------------------------------------------------------
void FlyoutWindow::OnClick(int x, int y) {
    POINT pt{ x, y };

    if (PtInRect(&m_playBtnRect, pt)) {
        if (!m_engine) return;
        if (!m_engine->HasMedia()) {
            auto file = util::OpenAudioFileDialog(m_hwnd);
            if (!file.empty()) {
                m_engine->OpenFile(file);
                m_engine->Play();
            }
        } else {
            m_engine->TogglePlayPause();
        }
        Render();
        return;
    }

    if (PtInRect(&m_openBtnRect, pt)) {
        auto file = util::OpenAudioFileDialog(m_hwnd);
        if (!file.empty() && m_engine) {
            m_engine->OpenFile(file);
            m_engine->Play();
            Render();
        }
        return;
    }

    if (PtInRect(&m_loopBtnRect, pt)) {
        if (m_engine) {
            m_engine->ToggleLooping();
            Render();
        }
        return;
    }
}

// ---------------------------------------------------------------------------
void FlyoutWindow::ShowNearTray(const RECT& trayRect) {
    Layout();

    int x = trayRect.left + (trayRect.right - trayRect.left) / 2 - m_width / 2;
    int y = trayRect.top - m_height - util::S(8, Dpi());

    HMONITOR hmon = MonitorFromRect(&trayRect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfoW(hmon, &mi);
    x = std::clamp(x, (int)mi.rcWork.left + 8,
                   (int)mi.rcWork.right - m_width - 8);
    y = std::clamp(y, (int)mi.rcWork.top + 8,
                   (int)mi.rcWork.bottom - m_height - 8);

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);

    Render();
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    // 让本窗口成为前台，这样点击外部会收到 WM_ACTIVATE 失活通知
    SetForegroundWindow(m_hwnd);

    // 兜底：捕获鼠标，即使 SetForegroundWindow 被系统拒绝，
    // 外部点击也会送到本窗口，由 WM_LBUTTONDOWN 里的边界检查关闭面板
    SetCapture(m_hwnd);

    m_visible = true;
}

void FlyoutWindow::Hide() {
    if (!m_visible) return;

    // 释放鼠标捕获
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }

    ShowWindow(m_hwnd, SW_HIDE);
    m_visible = false;
}

void FlyoutWindow::ToggleNearTray(const RECT& trayRect) {
    if (m_visible) Hide();
    else           ShowNearTray(trayRect);
}

void FlyoutWindow::Refresh() {
    Render();
}