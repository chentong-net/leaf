//
// Created by Chen Tong on 2026/6/2.
//

#include "LFAudioPlayer.h"
#include "plugin/LFPlugin.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

// Desktop backend is currently implemented for Windows only.
// macOS is intentionally left unsupported until we have a test device.

struct LFAudioPlayer::PlatformState {
    std::string alias;
    std::thread monitorThread;
    std::atomic<bool> monitorRunning{false};
};

namespace {

#if defined(_WIN32)

std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";

    const int wideCount = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.c_str(),
        static_cast<int>(utf8.size()),
        nullptr,
        0
    );
    if (wideCount <= 0) {
        return L"";
    }

    std::wstring wide(static_cast<size_t>(wideCount), L'\0');
    const int converted = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.c_str(),
        static_cast<int>(utf8.size()),
        wide.data(),
        wideCount
    );
    if (converted <= 0) {
        return L"";
    }
    return wide;
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";

    const int utf8Count = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (utf8Count <= 0) {
        return "";
    }

    std::string utf8(static_cast<size_t>(utf8Count), '\0');
    const int converted = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        utf8Count,
        nullptr,
        nullptr
    );
    if (converted <= 0) {
        return "";
    }
    return utf8;
}

std::wstring quoteMciString(const std::string& input) {
    std::wstring wide = utf8ToWide(input);
    if (wide.empty() && !input.empty()) {
        wide.assign(input.begin(), input.end());
    }

    std::wstring out = L"\"";
    for (wchar_t c : wide) {
        out += c == L'"' ? L'\'' : c;
    }
    out += L"\"";
    return out;
}

std::string mciErrorText(MCIERROR error) {
    if (error == 0) return "";
    wchar_t buffer[256]{};
    if (mciGetErrorStringW(error, buffer, sizeof(buffer) / sizeof(buffer[0]))) {
        return wideToUtf8(buffer);
    }
    return "mci_error_" + std::to_string(static_cast<unsigned long>(error));
}

bool sendMci(const std::wstring& command, std::string* error = nullptr) {
    MCIERROR result = mciSendStringW(command.c_str(), nullptr, 0, nullptr);
    if (result == 0) {
        if (error) error->clear();
        return true;
    }
    if (error) *error = mciErrorText(result);
    return false;
}

bool sendMci(const std::string& command, std::string* error = nullptr) {
    return sendMci(utf8ToWide(command), error);
}

std::string queryMci(const std::wstring& command) {
    wchar_t buffer[256]{};
    MCIERROR result = mciSendStringW(command.c_str(), buffer, sizeof(buffer) / sizeof(buffer[0]), nullptr);
    if (result != 0) {
        return "";
    }
    return wideToUtf8(buffer);
}

std::string queryMci(const std::string& command) {
    return queryMci(utf8ToWide(command));
}

double queryMciSeconds(const std::string& alias, const char* property) {
    std::string value = queryMci("status " + alias + " " + (property ? property : ""));
    if (value.empty()) return 0.0;
    try {
        return std::stod(value) / 1000.0;
    } catch (...) {
        return 0.0;
    }
}

bool queryMciPlaying(const std::string& alias) {
    return queryMci("status " + alias + " mode") == "playing";
}

#endif

void dispatchComplete(const LFAudioPlayerCompletionCallback& callback, const LFAudioPlayerEvent& event) {
    if (!callback) return;
    LFPluginCenter::dispatchToMain([callback, event]() {
        callback(event);
    });
}

void dispatchError(const LFAudioPlayerErrorCallback& callback, const LFAudioPlayerEvent& event) {
    if (!callback) return;
    LFPluginCenter::dispatchToMain([callback, event]() {
        callback(event);
    });
}

} // namespace

std::string LFAudioPlayer::generatePlayerId() {
    std::random_device rd;
    std::uniform_int_distribution<int> byteDist(0, 255);
    unsigned char bytes[16]{};
    for (auto& byte : bytes) {
        byte = static_cast<unsigned char>(byteDist(rd));
    }
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            oss << '-';
        }
    }
    return oss.str();
}

LFAudioPlayer::Ptr LFAudioPlayer::create() {
    return std::make_shared<LFAudioPlayer>(generatePlayerId());
}

LFAudioPlayer::LFAudioPlayer(std::string playerId)
    : m_playerId(playerId.empty() ? generatePlayerId() : std::move(playerId)),
      m_platform(std::make_unique<PlatformState>()) {
#if defined(_WIN32)
    m_platform->alias = "leaf_audio_" + m_playerId;
    for (char& c : m_platform->alias) {
        if (c == '-') c = '_';
    }
#endif
}

LFAudioPlayer::~LFAudioPlayer() {
    if (m_platform) {
        m_platform->monitorRunning = false;
        if (m_platform->monitorThread.joinable()) {
            m_platform->monitorThread.join();
        }
#if defined(_WIN32)
        if (!m_platform->alias.empty()) {
            sendMci("close " + m_platform->alias);
        }
#endif
    }
}

void LFAudioPlayer::setSource(const std::string& source) {
    std::string error;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_source = source;
        m_positionSeconds = 0.0;
        m_durationSeconds = 0.0;
        m_playing = false;
    }

#if defined(_WIN32)
    if (source.empty()) {
        return;
    }

    if (!m_platform->alias.empty()) {
        sendMci("close " + m_platform->alias);
    }

    if (!sendMci(L"open " + quoteMciString(source) + L" alias " + utf8ToWide(m_platform->alias), &error)) {
        LFAudioPlayerErrorCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            callback = m_onError;
        }
        LFAudioPlayerEvent event;
        event.playerId = m_playerId;
        event.ok = false;
        event.error = error.empty() ? "audio_open_failed" : error;
        dispatchError(callback, event);
        return;
    }

    sendMci("set " + m_platform->alias + " time format milliseconds");
    const double duration = queryMciSeconds(m_platform->alias, "length");
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_durationSeconds = duration;
    }
    setVolume(m_volume);
#else
    LFAudioPlayerErrorCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_onError;
    }
    LFAudioPlayerEvent event;
    event.playerId = m_playerId;
    event.ok = false;
    event.error = "desktop_audio_windows_only";
    dispatchError(callback, event);
#endif
}

void LFAudioPlayer::play() {
#if defined(_WIN32)
    if (!m_platform || m_platform->alias.empty()) return;

    std::string command = "play " + m_platform->alias;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_positionSeconds > 0.0) {
            command += " from " + std::to_string(static_cast<int>(m_positionSeconds * 1000.0));
        }
        m_playing = true;
    }

    std::string error;
    if (!sendMci(command, &error)) {
        LFAudioPlayerErrorCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_playing = false;
            callback = m_onError;
        }
        LFAudioPlayerEvent event;
        event.playerId = m_playerId;
        event.ok = false;
        event.error = error.empty() ? "audio_play_failed" : error;
        dispatchError(callback, event);
        return;
    }

    startEventListening();
#else
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playing = false;
#endif
}

void LFAudioPlayer::pause() {
#if defined(_WIN32)
    if (!m_platform || m_platform->alias.empty()) return;
    sendMci("pause " + m_platform->alias);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = queryMciSeconds(m_platform->alias, "position");
        m_playing = false;
    }
#else
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playing = false;
#endif
}

void LFAudioPlayer::stop() {
#if defined(_WIN32)
    if (!m_platform || m_platform->alias.empty()) return;
    sendMci("stop " + m_platform->alias);
    sendMci("seek " + m_platform->alias + " to start");
#endif
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = 0.0;
        m_playing = false;
    }
}

void LFAudioPlayer::seek(double positionSeconds) {
    if (positionSeconds < 0.0) positionSeconds = 0.0;
#if defined(_WIN32)
    bool shouldResumePlayback = false;
    bool seekSucceeded = true;
    if (m_platform && !m_platform->alias.empty()) {
        shouldResumePlayback = queryMciPlaying(m_platform->alias);
        seekSucceeded = sendMci("seek " + m_platform->alias + " to " + std::to_string(static_cast<int>(positionSeconds * 1000.0)));
    }
#endif
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = positionSeconds;
    }
#if defined(_WIN32)
    if (seekSucceeded && shouldResumePlayback) {
        play();
    }
#endif
}

void LFAudioPlayer::setLooping(bool looping) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_looping = looping;
}

void LFAudioPlayer::setVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_volume = volume;
    }
#if defined(_WIN32)
    if (m_platform && !m_platform->alias.empty()) {
        const int mciVolume = static_cast<int>(volume * 1000.0f);
        sendMci("setaudio " + m_platform->alias + " volume to " + std::to_string(mciVolume));
    }
#endif
}

double LFAudioPlayer::getDuration() const {
#if defined(_WIN32)
    if (m_platform && !m_platform->alias.empty()) {
        const double duration = queryMciSeconds(m_platform->alias, "length");
        std::lock_guard<std::mutex> lock(m_mutex);
        m_durationSeconds = duration;
        return duration;
    }
#endif
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_durationSeconds < 0.0 ? 0.0 : m_durationSeconds;
}

double LFAudioPlayer::getPosition() const {
#if defined(_WIN32)
    if (m_platform && !m_platform->alias.empty()) {
        const double position = queryMciSeconds(m_platform->alias, "position");
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = position;
        return position;
    }
#endif
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_positionSeconds;
}

bool LFAudioPlayer::isPlaying() const {
#if defined(_WIN32)
    if (m_platform && !m_platform->alias.empty()) {
        const bool playing = queryMciPlaying(m_platform->alias);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playing = playing;
        return playing;
    }
#endif
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playing;
}

void LFAudioPlayer::setOnComplete(LFAudioPlayerCompletionCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onComplete = std::move(callback);
}

void LFAudioPlayer::setOnError(LFAudioPlayerErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void LFAudioPlayer::startEventListening() {
#if defined(_WIN32)
    if (!m_platform || m_platform->monitorRunning) {
        return;
    }
    m_platform->monitorRunning = true;
    m_platform->monitorThread = std::thread([this]() {
        bool wasPlaying = false;
        while (m_platform && m_platform->monitorRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            if (!m_platform || m_platform->alias.empty()) {
                continue;
            }

            const bool playing = queryMciPlaying(m_platform->alias);
            const double position = queryMciSeconds(m_platform->alias, "position");
            const double duration = queryMciSeconds(m_platform->alias, "length");

            LFAudioPlayerCompletionCallback completeCallback;
            bool completed = false;
            bool looping = false;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_positionSeconds = position;
                m_durationSeconds = duration;
                m_playing = playing;
                looping = m_looping;
                completed = wasPlaying && !playing && duration > 0.0 && position >= duration - 0.15;
                if (completed) {
                    completeCallback = m_onComplete;
                    m_playing = looping;
                }
            }

            wasPlaying = playing;
            if (!completed) {
                continue;
            }

            LFAudioPlayerEvent event;
            event.playerId = m_playerId;
            event.ok = true;
            event.completed = true;
            event.positionSeconds = static_cast<float>(position);
            dispatchComplete(completeCallback, event);

            if (looping) {
                sendMci("seek " + m_platform->alias + " to start");
                sendMci("play " + m_platform->alias);
                wasPlaying = true;
            }
        }
    });
#endif
}
