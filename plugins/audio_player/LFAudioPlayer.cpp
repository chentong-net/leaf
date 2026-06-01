//
// Created by Chen Tong on 2026/6/2.
//

#include "LFAudioPlayer.h"
#include "LFJSONParser.h"
#include "plugin/LFNativeSender.h"
#include "plugin/LFPlugin.h"

#include <condition_variable>
#include <iomanip>
#include <random>
#include <sstream>

namespace {

std::string escapeJson(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}

std::string buildPlayerArgs(const std::string& playerId) {
    std::ostringstream oss;
    oss << "{\"playerId\":\"" << escapeJson(playerId) << "\"}";
    return oss.str();
}

std::string buildSetSourceArgs(const std::string& playerId, const std::string& source) {
    std::ostringstream oss;
    oss << "{"
        << "\"playerId\":\"" << escapeJson(playerId) << "\","
        << "\"source\":\"" << escapeJson(source) << "\""
        << "}";
    return oss.str();
}

std::string buildSeekArgs(const std::string& playerId, double positionSeconds) {
    if (positionSeconds < 0.0) positionSeconds = 0.0;
    std::ostringstream oss;
    oss << "{"
        << "\"playerId\":\"" << escapeJson(playerId) << "\","
        << "\"position\":" << positionSeconds
        << "}";
    return oss.str();
}

std::string buildBoolArgs(const std::string& playerId, const char* name, bool value) {
    std::ostringstream oss;
    oss << "{"
        << "\"playerId\":\"" << escapeJson(playerId) << "\","
        << "\"" << (name ? name : "value") << "\":" << (value ? "true" : "false")
        << "}";
    return oss.str();
}

std::string buildFloatArgs(const std::string& playerId, const char* name, float value) {
    std::ostringstream oss;
    oss << "{"
        << "\"playerId\":\"" << escapeJson(playerId) << "\","
        << "\"" << (name ? name : "value") << "\":" << value
        << "}";
    return oss.str();
}

void sendCommand(const std::string& method, const std::string& args) {
    LFNativeSender::getInstance().send(method, args, nullptr);
}

LFMethodResult sendBlocking(const std::string& method, const std::string& args) {
    struct State {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
        LFMethodResult out;
    };
    auto state = std::make_shared<State>();

    LFNativeSender::getInstance().send(method, args, [state](const LFMethodResult& result) {
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->out = result;
            state->done = true;
        }
        state->cv.notify_one();
    });

    std::unique_lock<std::mutex> lock(state->mutex);
    state->cv.wait_for(lock, std::chrono::milliseconds(300), [&]() {
        return state->done;
    });
    return state->out;
}

double numberField(const LFJSONObject::Ptr& json, const char* name, double fallback) {
    if (!json || !name || !json->contains(name)) return fallback;
    try {
        return json->at(name).asDouble();
    } catch (...) {
        return fallback;
    }
}

bool boolField(const LFJSONObject::Ptr& json, const char* name, bool fallback) {
    if (!json || !name || !json->contains(name)) return fallback;
    try {
        return json->at(name).asBool();
    } catch (...) {
        return fallback;
    }
}

std::string stringField(const LFJSONObject::Ptr& json, const char* name, const std::string& fallback = "") {
    if (!json || !name || !json->contains(name)) return fallback;
    try {
        return json->at(name).asString();
    } catch (...) {
        return fallback;
    }
}

void dispatchEventCallback(
        const LFAudioPlayerCompletionCallback& onComplete,
        const LFAudioPlayerErrorCallback& onError,
        const LFAudioPlayerEvent& event) {
    LFPluginCenter::dispatchToMain([onComplete, onError, event]() {
        if (event.completed) {
            if (onComplete) onComplete(event);
            return;
        }
        if (onError) onError(event);
    });
}

} // namespace

struct LFAudioPlayer::PlatformState {
};

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

void LFAudioPlayer::handleNativeEventResult(
        const std::weak_ptr<LFAudioPlayer>& weakPlayer,
        const std::string& fallbackPlayerId,
        const LFMethodResult& result) {
    auto player = weakPlayer.lock();
    if (!player) {
        return;
    }

    LFAudioPlayerEvent event;
    event.playerId = fallbackPlayerId;
    event.ok = result.ok;
    event.code = result.code;
    event.error = result.error;

    if (result.ok) {
        try {
            auto json = LFJSONParser::parse(result.data);
            event.playerId = stringField(json, "playerId", fallbackPlayerId);
            const std::string type = stringField(json, "type");
            event.completed = type == "complete";
            event.error = stringField(json, "error");
            event.code = static_cast<int32_t>(numberField(json, "code", 0.0));
            event.positionSeconds = static_cast<float>(numberField(json, "position", 0.0));
        } catch (...) {
            event.ok = false;
            event.error = "audio_event_parse_failed";
        }
    }

    if (event.playerId != player->m_playerId) {
        return;
    }

    LFAudioPlayerCompletionCallback onComplete;
    LFAudioPlayerErrorCallback onError;
    bool shouldListenAgain = false;
    {
        std::lock_guard<std::mutex> lock(player->m_mutex);
        player->m_eventListening = false;
        if (event.completed) {
            player->m_playing = player->m_looping;
            player->m_positionSeconds = event.positionSeconds;
        }
        onComplete = player->m_onComplete;
        onError = player->m_onError;
        shouldListenAgain = result.ok && (static_cast<bool>(player->m_onComplete) || static_cast<bool>(player->m_onError));
    }

    if (event.ok || !result.canceled) {
        dispatchEventCallback(onComplete, onError, event);
    }

    if (shouldListenAgain) {
        player->startEventListening();
    }
}

LFAudioPlayer::Ptr LFAudioPlayer::create() {
    return std::make_shared<LFAudioPlayer>(generatePlayerId());
}

LFAudioPlayer::LFAudioPlayer(std::string playerId)
    : m_playerId(playerId.empty() ? generatePlayerId() : std::move(playerId)) {
}

LFAudioPlayer::~LFAudioPlayer() {
    sendCommand("audio_player.dispose", buildPlayerArgs(m_playerId));
}

void LFAudioPlayer::setSource(const std::string& source) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_source = source;
        m_durationSeconds = -1.0;
        m_positionSeconds = 0.0;
        m_playing = false;
    }
    sendCommand("audio_player.set_source", buildSetSourceArgs(m_playerId, source));
}

void LFAudioPlayer::play() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playing = true;
    }
    sendCommand("audio_player.play", buildPlayerArgs(m_playerId));
}

void LFAudioPlayer::pause() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playing = false;
    }
    sendCommand("audio_player.pause", buildPlayerArgs(m_playerId));
}

void LFAudioPlayer::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playing = false;
        m_positionSeconds = 0.0;
    }
    sendCommand("audio_player.stop", buildPlayerArgs(m_playerId));
}

void LFAudioPlayer::seek(double positionSeconds) {
    if (positionSeconds < 0.0) positionSeconds = 0.0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_positionSeconds = positionSeconds;
    }
    sendCommand("audio_player.seek", buildSeekArgs(m_playerId, positionSeconds));
}

void LFAudioPlayer::setLooping(bool looping) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_looping = looping;
    }
    sendCommand("audio_player.set_looping", buildBoolArgs(m_playerId, "looping", looping));
}

void LFAudioPlayer::setVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_volume = volume;
    }
    sendCommand("audio_player.set_volume", buildFloatArgs(m_playerId, "volume", volume));
}

double LFAudioPlayer::getDuration() const {
    const LFMethodResult result = sendBlocking("audio_player.get_duration", buildPlayerArgs(m_playerId));
    if (result.ok) {
        try {
            auto json = LFJSONParser::parse(result.data);
            const double duration = numberField(json, "duration", 0.0);
            std::lock_guard<std::mutex> lock(m_mutex);
            m_durationSeconds = duration;
            return duration;
        } catch (...) {
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_durationSeconds < 0.0 ? 0.0 : m_durationSeconds;
}

double LFAudioPlayer::getPosition() const {
    const LFMethodResult result = sendBlocking("audio_player.get_position", buildPlayerArgs(m_playerId));
    if (result.ok) {
        try {
            auto json = LFJSONParser::parse(result.data);
            const double position = numberField(json, "position", 0.0);
            std::lock_guard<std::mutex> lock(m_mutex);
            m_positionSeconds = position;
            return position;
        } catch (...) {
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_positionSeconds;
}

bool LFAudioPlayer::isPlaying() const {
    const LFMethodResult result = sendBlocking("audio_player.is_playing", buildPlayerArgs(m_playerId));
    if (result.ok) {
        try {
            auto json = LFJSONParser::parse(result.data);
            const bool playing = boolField(json, "playing", false);
            std::lock_guard<std::mutex> lock(m_mutex);
            m_playing = playing;
            return playing;
        } catch (...) {
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playing;
}

void LFAudioPlayer::setOnComplete(LFAudioPlayerCompletionCallback callback) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onComplete = std::move(callback);
    }
    startEventListening();
}

void LFAudioPlayer::setOnError(LFAudioPlayerErrorCallback callback) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onError = std::move(callback);
    }
    startEventListening();
}

void LFAudioPlayer::startEventListening() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_eventListening) {
            return;
        }
        if (!m_onComplete && !m_onError) {
            return;
        }
        m_eventListening = true;
    }

    const std::string playerId = m_playerId;
    const std::weak_ptr<LFAudioPlayer> weakSelf = weak_from_this();
    if (weakSelf.expired()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventListening = false;
        return;
    }

    LFNativeSender::getInstance().send(
        "audio_player.listen",
        buildPlayerArgs(playerId),
        [weakSelf, playerId](const LFMethodResult& result) {
            handleNativeEventResult(weakSelf, playerId, result);
        }
    );
}
