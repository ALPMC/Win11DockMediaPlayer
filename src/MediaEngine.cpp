#include "Common.h"
#include "MediaEngine.h"

using namespace winrt::Windows::Media::Core;
using namespace winrt::Windows::Media::Playback;

MediaEngine::MediaEngine() {
    m_player = MediaPlayer();
    m_player.AudioCategory(MediaPlayerAudioCategory::Media);

    m_session = m_player.PlaybackSession();

    m_session.PlaybackStateChanged(
        [this](auto&& sender, auto&&) {
            auto st = sender.PlaybackState();
            m_isPlaying = (st == MediaPlaybackState::Playing);
            if (m_notifyHwnd) {
                PostMessageW(m_notifyHwnd, WM_MEDIA_STATE, 0, 0);
            }
        });
}

MediaEngine::~MediaEngine() {
    if (m_player) {
        try { m_player.Pause(); } catch (...) {}
        m_player.Source(nullptr);
    }
}

bool MediaEngine::OpenFile(const std::wstring& path) {
    try {
        m_player.Source(nullptr);

        auto uri = winrt::Windows::Foundation::Uri(util::PathToFileUri(path));
        auto source = MediaSource::CreateFromUri(uri);

        m_player.Source(source);
        m_currentFile = path;
        m_hasMedia = true;

        // 每次打开新文件时重新应用循环设置
        m_player.IsLoopingEnabled(m_isLooping);
        return true;
    } catch (...) {
        m_hasMedia = false;
        m_currentFile.clear();
        return false;
    }
}

void MediaEngine::Play() {
    if (m_hasMedia && m_player) m_player.Play();
}

void MediaEngine::Pause() {
    if (m_hasMedia && m_player) m_player.Pause();
}

void MediaEngine::TogglePlayPause() {
    if (!m_hasMedia) return;
    if (m_isPlaying) m_player.Pause();
    else             m_player.Play();
}

// ★ 新增：切换循环播放
void MediaEngine::ToggleLooping() {
    m_isLooping = !m_isLooping;
    if (m_player) {
        try {
            m_player.IsLoopingEnabled(m_isLooping);
        } catch (...) {
            // 某些特殊媒体源可能不支持，忽略
        }
    }
}