//
// Created by Chen Tong on 2026/6/2.
// macOS desktop audio player implementation using AVAudioPlayer
//

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include "LFAudioPlayer.h"
#include "plugin/LFPlugin.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>

struct LFAudioPlayer::PlatformState {
    AVAudioPlayer* player;
    std::thread monitorThread;
    std::atomic<bool> monitorRunning{false};

    PlatformState() : player(nil) {}
};

namespace {

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
}

LFAudioPlayer::~LFAudioPlayer() {
    if (m_platform) {
        m_platform->monitorRunning = false;
        if (m_platform->monitorThread.joinable()) {
            m_platform->monitorThread.join();
        }
        if (m_platform->player) {
            [m_platform->player stop];
            m_platform->player = nil;
        }
    }
}

void LFAudioPlayer::setSource(const std::string& source) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_source = source;
        m_positionSeconds = 0.0;
        m_durationSeconds = 0.0;
        m_playing = false;
    }

    if (!m_platform) return;
    if (source.empty()) return;

    if (m_platform->player) {
        [m_platform->player stop];
        m_platform->player = nil;
    }

    NSString* path = [NSString stringWithUTF8String:source.c_str()];
    NSURL* url = [NSURL fileURLWithPath:path];
    NSError* error = nil;
    AVAudioPlayer* player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];

    if (!player) {
        LFAudioPlayerErrorCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            callback = m_onError;
        }
        LFAudioPlayerEvent event;
        event.playerId = m_playerId;
        event.ok = false;
        event.error = error ? std::string([[error localizedDescription] UTF8String]) : "audio_open_failed";
        dispatchError(callback, event);
        return;
    }

    player.numberOfLoops = 0;
    [player prepareToPlay];

    m_platform->player = player;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_durationSeconds = player.duration;
    }
    setVolume(m_volume);
}

void LFAudioPlayer::play() {
    if (!m_platform || !m_platform->player) return;

    AVAudioPlayer* player = m_platform->player;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_positionSeconds > 0.0 && m_positionSeconds < player.duration) {
            player.currentTime = m_positionSeconds;
        }
        m_playing = true;
    }

    [player play];

    startEventListening();
}

void LFAudioPlayer::pause() {
    if (!m_platform || !m_platform->player) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playing = false;
        return;
    }

    AVAudioPlayer* player = m_platform->player;
    [player pause];
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = player.currentTime;
        m_playing = false;
    }
}

void LFAudioPlayer::stop() {
    if (m_platform && m_platform->player) {
        AVAudioPlayer* player = m_platform->player;
        [player stop];
        player.currentTime = 0.0;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = 0.0;
        m_playing = false;
    }
}

void LFAudioPlayer::seek(double positionSeconds) {
    if (positionSeconds < 0.0) positionSeconds = 0.0;

    bool shouldResumePlayback = false;
    if (m_platform && m_platform->player) {
        AVAudioPlayer* player = m_platform->player;
        shouldResumePlayback = player.isPlaying;
        if (positionSeconds > player.duration) {
            positionSeconds = player.duration;
        }
        player.currentTime = positionSeconds;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = positionSeconds;
    }
    if (shouldResumePlayback) {
        play();
    }
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
    if (m_platform && m_platform->player) {
        m_platform->player.volume = volume;
    }
}

double LFAudioPlayer::getDuration() const {
    if (m_platform && m_platform->player) {
        const double duration = m_platform->player.duration;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_durationSeconds = duration;
        return duration;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_durationSeconds < 0.0 ? 0.0 : m_durationSeconds;
}

double LFAudioPlayer::getPosition() const {
    if (m_platform && m_platform->player) {
        const double position = m_platform->player.currentTime;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = position;
        return position;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_positionSeconds;
}

bool LFAudioPlayer::isPlaying() const {
    if (m_platform && m_platform->player) {
        const bool playing = m_platform->player.isPlaying;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playing = playing;
        return playing;
    }
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
    if (!m_platform || m_platform->monitorRunning) {
        return;
    }
    m_platform->monitorRunning = true;
    m_platform->monitorThread = std::thread([this]() {
        bool wasPlaying = false;
        while (m_platform && m_platform->monitorRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            if (!m_platform || !m_platform->player) {
                continue;
            }

            AVAudioPlayer* player = m_platform->player;
            const bool playing = player.isPlaying;
            const double position = player.currentTime;
            const double duration = player.duration;

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

            if (looping && m_platform && m_platform->player) {
                m_platform->player.currentTime = 0.0;
                [m_platform->player play];
                wasPlaying = true;
            }
        }
    });
}
