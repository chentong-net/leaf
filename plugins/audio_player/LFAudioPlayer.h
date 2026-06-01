//
// Created by Chen Tong on 2026/6/2.
//

#ifndef LEAF_LFAUDIOPLAYER_H
#define LEAF_LFAUDIOPLAYER_H

#include "LFDef.h"
#include "plugin/LFMethodTypes.h"

struct LFAudioPlayerEvent {
    std::string playerId;
    std::string error;
    bool ok = false;
    bool completed = false;
    int32_t code = 0;
    float positionSeconds = 0.0f;
};

using LFAudioPlayerErrorCallback = std::function<void(const LFAudioPlayerEvent&)>;
using LFAudioPlayerCompletionCallback = std::function<void(const LFAudioPlayerEvent&)>;

class LFAudioPlayer : public std::enable_shared_from_this<LFAudioPlayer> {
public:
    using Ptr = std::shared_ptr<LFAudioPlayer>;

    static Ptr create();

    explicit LFAudioPlayer(std::string playerId);
    ~LFAudioPlayer();

    const std::string& getPlayerId() const { return m_playerId; }

    void setSource(const std::string& source);
    void play();
    void pause();
    void stop();
    void seek(double positionSeconds);

    void setLooping(bool looping);
    void setVolume(float volume);

    double getDuration() const;
    double getPosition() const;
    bool isPlaying() const;

    void setOnComplete(LFAudioPlayerCompletionCallback callback);
    void setOnError(LFAudioPlayerErrorCallback callback);

private:
    struct PlatformState;

    static std::string generatePlayerId();
    static void handleNativeEventResult(
            const std::weak_ptr<LFAudioPlayer>& weakPlayer,
            const std::string& fallbackPlayerId,
            const LFMethodResult& result);

    void startEventListening();

    std::string m_playerId;
    std::string m_source;
    bool m_looping = false;
    float m_volume = 1.0f;
    mutable double m_durationSeconds = -1.0;
    mutable double m_positionSeconds = 0.0;
    mutable bool m_playing = false;
    bool m_eventListening = false;
    mutable std::mutex m_mutex;
    std::unique_ptr<PlatformState> m_platform;
    LFAudioPlayerCompletionCallback m_onComplete;
    LFAudioPlayerErrorCallback m_onError;
};

#endif // LEAF_LFAUDIOPLAYER_H
