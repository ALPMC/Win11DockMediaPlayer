#include "Common.h"
#include "TrayIcon.h"

using Microsoft::WRL::ComPtr;

TrayIcon::TrayIcon() = default;
TrayIcon::~TrayIcon() { Destroy(); }

// ---------------------------------------------------------------------------
// 从 exe 内嵌资源读取 PNG → HICON
// ---------------------------------------------------------------------------
HICON TrayIcon::LoadHIconFromResource(const wchar_t* resName, int size) {
    util::Log(L"[Tray] LoadHIconFromResource('%ls', %d)", resName, size);

    auto png = util::LoadResourceBytes(resName);
    util::Log(L"[Tray]   资源字节数 = %zu", png.size());
    if (png.empty()) return nullptr;

    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) { util::Log(L"[Tray]   CoCreateInstance(WIC) 失败"); return nullptr; }

    ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(&stream))) return nullptr;
    if (FAILED(stream->InitializeFromMemory(png.data(), (DWORD)png.size()))) return nullptr;

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr,
                                                 WICDecodeMetadataCacheOnLoad,
                                                 &decoder)))
        return nullptr;

    ComPtr<IWICBitmapFrameDecode> frame;
    decoder->GetFrame(0, &frame);

    ComPtr<IWICFormatConverter> converter;
    factory->CreateFormatConverter(&converter);
    converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
                          WICBitmapDitherTypeNone, nullptr, 0.0,
                          WICBitmapPaletteTypeCustom);

    ComPtr<IWICBitmapScaler> scaler;
    factory->CreateBitmapScaler(&scaler);
    scaler->Initialize(converter.Get(), size, size,
                       WICBitmapInterpolationModeFant);

    std::vector<BYTE> pixels((size_t)size * size * 4);
    WICRect rc{ 0, 0, size, size };
    scaler->CopyPixels(&rc, size * 4, (UINT)pixels.size(), pixels.data());

    for (size_t i = 0; i < pixels.size(); i += 4) {
        BYTE a = pixels[i + 3];
        pixels[i + 0] = (BYTE)(pixels[i + 0] * a / 255);
        pixels[i + 1] = (BYTE)(pixels[i + 1] * a / 255);
        pixels[i + 2] = (BYTE)(pixels[i + 2] * a / 255);
    }

    HBITMAP hColor = CreateBitmap(size, size, 1, 32, pixels.data());
    HBITMAP hMask  = CreateBitmap(size, size, 1, 1, nullptr);

    ICONINFO ii{};
    ii.fIcon    = TRUE;
    ii.hbmColor = hColor;
    ii.hbmMask  = hMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hColor);
    DeleteObject(hMask);
    util::Log(L"[Tray]   HICON=%p", hIcon);
    return hIcon;
}

// ---------------------------------------------------------------------------
// 创建托盘图标 —— 只往系统注册，不再创建自己的窗口
// ---------------------------------------------------------------------------
bool TrayIcon::Create(HWND hwnd, const wchar_t* resName,
                      const std::wstring& tooltip)
{
    util::Log(L"[Tray] ===== Create 开始, 使用外部 HWND=%p =====", hwnd);

    if (!hwnd) {
        util::Log(L"[Tray] 传入的 HWND 为空");
        return false;
    }
    m_hwnd = hwnd;

    // 加载图标
    m_hIcon = LoadHIconFromResource(resName, GetSystemMetrics(SM_CXSMICON));
    if (!m_hIcon) {
        util::Log(L"[Tray] 资源图标加载失败，回退到系统默认图标");
        m_hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    util::Log(L"[Tray] 最终 HICON=%p", m_hIcon);

    // 清理残留
    {
        NOTIFYICONDATAW cleanup{};
        cleanup.cbSize = sizeof(cleanup);
        cleanup.hWnd   = m_hwnd;
        cleanup.uID    = 1;
        Shell_NotifyIconW(NIM_DELETE, &cleanup);
    }

    // 填充 nid
    m_nid.cbSize           = sizeof(m_nid);
    m_nid.hWnd             = m_hwnd;
    m_nid.uID              = 1;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON_;
    m_nid.hIcon            = m_hIcon;
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);

    util::Log(L"[Tray] NIM_ADD: hwnd=%p uid=%u cbSize=%u",
              m_nid.hWnd, m_nid.uID, (unsigned)m_nid.cbSize);

    // NIM_ADD
    SetLastError(0);
    BOOL addOk = Shell_NotifyIconW(NIM_ADD, &m_nid);
    DWORD addErr = GetLastError();
    util::Log(L"[Tray] NIM_ADD ok=%d err=%lu", addOk, addErr);

    if (!addOk) {
        util::Log(L"[Tray] NIM_ADD 首次失败，删除后重试");
        Shell_NotifyIconW(NIM_DELETE, &m_nid);

        SetLastError(0);
        addOk = Shell_NotifyIconW(NIM_ADD, &m_nid);
        addErr = GetLastError();
        util::Log(L"[Tray] NIM_ADD 重试 ok=%d err=%lu", addOk, addErr);

        if (!addOk) {
            util::Log(L"[Tray] NIM_ADD 两次都失败");
            return false;
        }
    }

    m_registered = true;
    util::Log(L"[Tray] ===== Create 成功 =====");
    return true;
}

void TrayIcon::Destroy() {
    if (m_registered) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_registered = false;
    }
    if (m_hIcon) { DestroyIcon(m_hIcon); m_hIcon = nullptr; }
    // 不再 DestroyWindow —— 窗口是 FlyoutWindow 的
    m_hwnd = nullptr;
}

// ---------------------------------------------------------------------------
// 由 FlyoutWindow 转发进来
// ---------------------------------------------------------------------------
void TrayIcon::HandleTrayMessage(WPARAM wp, LPARAM lp) {
    if (lp == WM_LBUTTONUP && OnLeftClick)        OnLeftClick();
    else if (lp == WM_RBUTTONUP && OnRightClick)  OnRightClick();
}

RECT TrayIcon::GetIconRect() const {
    NOTIFYICONIDENTIFIER nii{};
    nii.cbSize = sizeof(nii);
    nii.hWnd   = m_hwnd;
    nii.uID    = 1;
    RECT r{};
    Shell_NotifyIconGetRect(&nii, &r);
    return r;
}