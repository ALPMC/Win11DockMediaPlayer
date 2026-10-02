#pragma once

#include "Common.h"

class MediaEngine {
public:
    MediaEngine();
    ~MediaEngine();

    void SetNotifyWindow(HWND hwnd) { m_notifyHwnd = hwnd; }

    bool OpenFile(const std::wstring& path);
    void Play();
    void Pause();
    void TogglePlayPause();

    bool IsPlaying() const { return m_isPlaying; }
    bool HasMedia()  const { return m_hasMedia; }
    const std::wstring& CurrentFile() const { return m_currentFile; }

    // ★ 新增：循环播放
    bool IsLooping() const { return m_isLooping; }
    void ToggleLooping();

private:
    HWND m_notifyHwnd = nullptr;

    winrt::Windows::Media::Playback::MediaPlayer m_player{ nullptr };
    winrt::Windows::Media::Playback::MediaPlaybackSession m_session{ nullptr };

    bool m_isPlaying = false;
    bool m_hasMedia  = false;
    bool m_isLooping = false;             // ★ 新增
    std::wstring m_currentFile;
};