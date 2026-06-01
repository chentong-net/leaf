package net.chentong.leaf.android.plugin.audioplayer;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;

import net.chentong.leaf.android.plugin.LeafPlugin;

import org.json.JSONObject;

import java.util.ArrayDeque;
import java.util.Map;
import java.util.Queue;
import java.util.concurrent.ConcurrentHashMap;

public class AudioPlayerPlugin implements LeafPlugin {
    private final Context context;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Map<String, PlayerRecord> players = new ConcurrentHashMap<>();

    public AudioPlayerPlugin(Context context) {
        this.context = context == null ? null : context.getApplicationContext();
    }

    @Override
    public String pluginName() {
        return "AudioPlayerPlugin";
    }

    @Override
    public boolean canHandle(String method) {
        return method != null && method.startsWith("audio_player.");
    }

    @Override
    public void onMethodCall(LeafMethodCall call, Result result) {
        String method = call.getMethod();
        try {
            if ("audio_player.set_source".equals(method)) {
                runOnMain(call, result, () -> handleSetSource(call, result));
                return;
            }
            if ("audio_player.play".equals(method)) {
                runOnMain(call, result, () -> handlePlay(call, result));
                return;
            }
            if ("audio_player.pause".equals(method)) {
                runOnMain(call, result, () -> handlePause(call, result));
                return;
            }
            if ("audio_player.stop".equals(method)) {
                runOnMain(call, result, () -> handleStop(call, result));
                return;
            }
            if ("audio_player.seek".equals(method)) {
                runOnMain(call, result, () -> handleSeek(call, result));
                return;
            }
            if ("audio_player.set_looping".equals(method)) {
                runOnMain(call, result, () -> handleSetLooping(call, result));
                return;
            }
            if ("audio_player.set_volume".equals(method)) {
                runOnMain(call, result, () -> handleSetVolume(call, result));
                return;
            }
            if ("audio_player.get_duration".equals(method)) {
                runOnMain(call, result, () -> handleGetDuration(call, result));
                return;
            }
            if ("audio_player.get_position".equals(method)) {
                runOnMain(call, result, () -> handleGetPosition(call, result));
                return;
            }
            if ("audio_player.is_playing".equals(method)) {
                runOnMain(call, result, () -> handleIsPlaying(call, result));
                return;
            }
            if ("audio_player.listen".equals(method)) {
                runOnMain(call, result, () -> handleListen(call, result));
                return;
            }
            if ("audio_player.dispose".equals(method)) {
                runOnMain(call, result, () -> handleDispose(call, result));
                return;
            }
            result.error(call.getRequestId(), -404, "method_not_implemented", false);
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -500, errorText(throwable), false);
        }
    }

    public void release() {
        mainHandler.post(() -> {
            for (PlayerRecord record : players.values()) {
                releaseRecord(record);
            }
            players.clear();
        });
    }

    private void runOnMain(LeafMethodCall call, Result result, ThrowingRunnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            safeRun(call, result, runnable);
            return;
        }
        mainHandler.post(() -> safeRun(call, result, runnable));
    }

    private void safeRun(LeafMethodCall call, Result result, ThrowingRunnable runnable) {
        try {
            runnable.run();
        } catch (Throwable throwable) {
            result.error(call.getRequestId(), -500, errorText(throwable), false);
        }
    }

    private void handleSetSource(LeafMethodCall call, Result result) throws Exception {
        JSONObject args = parseArgs(call.getArgs());
        String playerId = requirePlayerId(args);
        String source = args.optString("source", "");
        if (source.isEmpty()) {
            result.error(call.getRequestId(), -1, "source_empty", false);
            return;
        }

        PlayerRecord record = getOrCreateRecord(playerId);
        preparePlayer(record, source);
        result.success(call.getRequestId(), buildPlayerJson(playerId).toString());
    }

    private void handlePlay(LeafMethodCall call, Result result) throws Exception {
        PlayerRecord record = requireRecord(call, result);
        if (record == null) return;
        if (record.mediaPlayer == null) {
            result.error(call.getRequestId(), -2, "source_not_set", false);
            return;
        }
        record.mediaPlayer.start();
        result.success(call.getRequestId(), buildPlayerJson(record.playerId).toString());
    }

    private void handlePause(LeafMethodCall call, Result result) throws Exception {
        PlayerRecord record = requireRecord(call, result);
        if (record == null) return;
        if (record.mediaPlayer != null && record.mediaPlayer.isPlaying()) {
            record.mediaPlayer.pause();
        }
        result.success(call.getRequestId(), buildPlayerJson(record.playerId).toString());
    }

    private void handleStop(LeafMethodCall call, Result result) throws Exception {
        PlayerRecord record = requireRecord(call, result);
        if (record == null) return;
        if (record.mediaPlayer != null) {
            if (record.mediaPlayer.isPlaying()) {
                record.mediaPlayer.pause();
            }
            record.mediaPlayer.seekTo(0);
        }
        result.success(call.getRequestId(), buildPlayerJson(record.playerId).toString());
    }

    private void handleSeek(LeafMethodCall call, Result result) throws Exception {
        JSONObject args = parseArgs(call.getArgs());
        PlayerRecord record = requireRecord(args, call, result);
        if (record == null) return;
        double position = Math.max(0.0, args.optDouble("position", 0.0));
        if (record.mediaPlayer != null) {
            record.mediaPlayer.seekTo((int) Math.round(position * 1000.0));
        }
        result.success(call.getRequestId(), buildPlayerJson(record.playerId).toString());
    }

    private void handleSetLooping(LeafMethodCall call, Result result) throws Exception {
        JSONObject args = parseArgs(call.getArgs());
        PlayerRecord record = requireRecord(args, call, result);
        if (record == null) return;
        record.looping = args.optBoolean("looping", false);
        if (record.mediaPlayer != null) {
            record.mediaPlayer.setLooping(record.looping);
        }
        result.success(call.getRequestId(), buildPlayerJson(record.playerId).toString());
    }

    private void handleSetVolume(LeafMethodCall call, Result result) throws Exception {
        JSONObject args = parseArgs(call.getArgs());
        PlayerRecord record = requireRecord(args, call, result);
        if (record == null) return;
        double volume = args.optDouble("volume", 1.0);
        if (volume < 0.0) volume = 0.0;
        if (volume > 1.0) volume = 1.0;
        record.volume = (float) volume;
        if (record.mediaPlayer != null) {
            record.mediaPlayer.setVolume(record.volume, record.volume);
        }
        result.success(call.getRequestId(), buildPlayerJson(record.playerId).toString());
    }

    private void handleGetDuration(LeafMethodCall call, Result result) throws Exception {
        PlayerRecord record = requireRecord(call, result);
        if (record == null) return;
        JSONObject out = buildPlayerJson(record.playerId);
        out.put("duration", durationSeconds(record));
        result.success(call.getRequestId(), out.toString());
    }

    private void handleGetPosition(LeafMethodCall call, Result result) throws Exception {
        PlayerRecord record = requireRecord(call, result);
        if (record == null) return;
        JSONObject out = buildPlayerJson(record.playerId);
        out.put("position", positionSeconds(record));
        result.success(call.getRequestId(), out.toString());
    }

    private void handleIsPlaying(LeafMethodCall call, Result result) throws Exception {
        PlayerRecord record = requireRecord(call, result);
        if (record == null) return;
        JSONObject out = buildPlayerJson(record.playerId);
        out.put("playing", record.mediaPlayer != null && record.mediaPlayer.isPlaying());
        result.success(call.getRequestId(), out.toString());
    }

    private void handleListen(LeafMethodCall call, Result result) throws Exception {
        JSONObject args = parseArgs(call.getArgs());
        String playerId = requirePlayerId(args);
        PlayerRecord record = getOrCreateRecord(playerId);

        if (record.pendingListener != null) {
            result.error(call.getRequestId(), -11, "listen_in_progress", false);
            return;
        }

        if (!record.events.isEmpty()) {
            String event = record.events.poll();
            result.success(call.getRequestId(), event);
            return;
        }

        record.pendingListener = new PendingListener(call.getRequestId(), result);
    }

    private void handleDispose(LeafMethodCall call, Result result) throws Exception {
        JSONObject args = parseArgs(call.getArgs());
        String playerId = requirePlayerId(args);
        PlayerRecord record = players.remove(playerId);
        if (record != null) {
            releaseRecord(record);
        }
        result.success(call.getRequestId(), buildPlayerJson(playerId).toString());
    }

    private PlayerRecord getOrCreateRecord(String playerId) {
        PlayerRecord existing = players.get(playerId);
        if (existing != null) {
            return existing;
        }
        PlayerRecord record = new PlayerRecord();
        record.playerId = playerId;
        PlayerRecord raced = players.putIfAbsent(playerId, record);
        return raced == null ? record : raced;
    }

    private void preparePlayer(PlayerRecord record, String source) throws Exception {
        releaseMediaPlayerOnly(record);

        MediaPlayer player = new MediaPlayer();
        player.setAudioAttributes(new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
            .build());
        player.setLooping(record.looping);
        player.setVolume(record.volume, record.volume);
        player.setOnCompletionListener(mp -> {
            if (record.looping) {
                return;
            }
            enqueueEvent(record, "complete", "", 0);
        });
        player.setOnErrorListener((mp, what, extra) -> {
            enqueueEvent(record, "error", "media_player_error_" + what + "_" + extra, what);
            return true;
        });

        if (isUriSource(source)) {
            player.setDataSource(context, Uri.parse(source));
        } else {
            player.setDataSource(source);
        }
        player.prepare();

        record.source = source;
        record.mediaPlayer = player;
    }

    private void enqueueEvent(PlayerRecord record, String type, String error, int code) {
        if (record == null) return;
        try {
            JSONObject event = buildPlayerJson(record.playerId);
            event.put("type", type == null ? "" : type);
            event.put("error", error == null ? "" : error);
            event.put("code", code);
            event.put("position", positionSeconds(record));
            String eventText = event.toString();
            if (record.pendingListener != null) {
                PendingListener pending = record.pendingListener;
                record.pendingListener = null;
                pending.result.success(pending.requestId, eventText);
            } else {
                record.events.add(eventText);
            }
        } catch (Throwable ignored) {
        }
    }

    private PlayerRecord requireRecord(LeafMethodCall call, Result result) throws Exception {
        return requireRecord(parseArgs(call.getArgs()), call, result);
    }

    private PlayerRecord requireRecord(JSONObject args, LeafMethodCall call, Result result) throws Exception {
        String playerId = requirePlayerId(args);
        PlayerRecord record = players.get(playerId);
        if (record == null) {
            result.error(call.getRequestId(), -404, "player_not_found", false);
            return null;
        }
        return record;
    }

    private String requirePlayerId(JSONObject args) throws Exception {
        String playerId = args.optString("playerId", "");
        if (playerId.isEmpty()) {
            throw new IllegalArgumentException("player_id_empty");
        }
        return playerId;
    }

    private JSONObject buildPlayerJson(String playerId) throws Exception {
        JSONObject out = new JSONObject();
        out.put("playerId", playerId == null ? "" : playerId);
        return out;
    }

    private JSONObject parseArgs(String args) throws Exception {
        if (args == null || args.trim().isEmpty()) {
            return new JSONObject();
        }
        return new JSONObject(args);
    }

    private String errorText(Throwable throwable) {
        if (throwable == null) {
            return "unknown_error";
        }
        String message = throwable.getMessage();
        if (message == null || message.isEmpty()) {
            return throwable.getClass().getSimpleName();
        }
        return message;
    }

    private boolean isUriSource(String source) {
        return source.startsWith("content://")
            || source.startsWith("android.resource://")
            || source.startsWith("file://")
            || source.startsWith("http://")
            || source.startsWith("https://");
    }

    private double durationSeconds(PlayerRecord record) {
        try {
            if (record == null || record.mediaPlayer == null) return 0.0;
            return Math.max(0, record.mediaPlayer.getDuration()) / 1000.0;
        } catch (Throwable ignored) {
            return 0.0;
        }
    }

    private double positionSeconds(PlayerRecord record) {
        try {
            if (record == null || record.mediaPlayer == null) return 0.0;
            return Math.max(0, record.mediaPlayer.getCurrentPosition()) / 1000.0;
        } catch (Throwable ignored) {
            return 0.0;
        }
    }

    private void releaseRecord(PlayerRecord record) {
        if (record == null) return;
        releaseMediaPlayerOnly(record);
        if (record.pendingListener != null) {
            record.pendingListener.result.error(record.pendingListener.requestId, -1, "player_disposed", true);
            record.pendingListener = null;
        }
        record.events.clear();
    }

    private void releaseMediaPlayerOnly(PlayerRecord record) {
        if (record == null || record.mediaPlayer == null) return;
        try {
            record.mediaPlayer.setOnCompletionListener(null);
            record.mediaPlayer.setOnErrorListener(null);
            record.mediaPlayer.release();
        } catch (Throwable ignored) {
        }
        record.mediaPlayer = null;
    }

    private static final class PendingListener {
        final int requestId;
        final Result result;

        PendingListener(int requestId, Result result) {
            this.requestId = requestId;
            this.result = result;
        }
    }

    private interface ThrowingRunnable {
        void run() throws Exception;
    }

    private static final class PlayerRecord {
        String playerId;
        String source;
        boolean looping;
        float volume = 1.0f;
        MediaPlayer mediaPlayer;
        PendingListener pendingListener;
        final Queue<String> events = new ArrayDeque<>();
    }
}
