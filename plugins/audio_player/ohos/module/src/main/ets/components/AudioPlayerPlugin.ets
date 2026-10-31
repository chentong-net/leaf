import { fileIo } from '@kit.CoreFileKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { media } from '@kit.MediaKit';
import { LeafMethodCall, LeafPlugin, LeafResult } from 'leaf_plugin';

const REQUEST_ERROR_INVALID_ARGS = -1;
const REQUEST_ERROR_SOURCE_NOT_SET = -2;
const REQUEST_ERROR_PLAYER_NOT_FOUND = -404;
const REQUEST_ERROR_INTERNAL = -500;
const REQUEST_ERROR_LISTEN_IN_PROGRESS = -11;
const REQUEST_ERROR_SOURCE_REPLACED = -12;
const MAX_PENDING_EVENTS = 32;

class PlayerArgs {
  playerId: string = '';
  source: string = '';
  position: number = 0.0;
  looping: boolean = false;
  volume: number = 1.0;
}

class PendingListener {
  requestId: number = 0;
  result: LeafResult;

  constructor(requestId: number, result: LeafResult) {
    this.requestId = requestId;
    this.result = result;
  }
}

class PlayerPayload {
  playerId: string = '';
  duration: number = 0.0;
  position: number = 0.0;
  playing: boolean = false;
}

class AudioEventPayload {
  playerId: string = '';
  type: string = '';
  error: string = '';
  code: number = 0;
  position: number = 0.0;
}

class PlayerRecord {
  readonly playerId: string;
  player: media.AVPlayer | null = null;
  file: fileIo.File | null = null;
  source: string = '';
  looping: boolean = false;
  volume: number = 1.0;
  generation: number = 0;
  events: string[] = [];
  pendingListener: PendingListener | null = null;
  pendingSourceResult: LeafResult | null = null;
  pendingSourceRequestId: number = 0;

  constructor(playerId: string) {
    this.playerId = playerId;
  }
}

export class AudioPlayerPlugin implements LeafPlugin {
  private readonly players: Map<string, PlayerRecord> = new Map<string, PlayerRecord>();

  pluginName(): string {
    return 'AudioPlayerPlugin';
  }

  canHandle(method: string): boolean {
    return method.startsWith('audio_player.');
  }

  onMethodCall(call: LeafMethodCall, result: LeafResult): void {
    try {
      if (call.method === 'audio_player.set_source') {
        this.handleSetSource(call, result);
        return;
      }
      if (call.method === 'audio_player.play') {
        this.handlePlay(call, result);
        return;
      }
      if (call.method === 'audio_player.pause') {
        this.handlePause(call, result);
        return;
      }
      if (call.method === 'audio_player.stop') {
        this.handleStop(call, result);
        return;
      }
      if (call.method === 'audio_player.seek') {
        this.handleSeek(call, result);
        return;
      }
      if (call.method === 'audio_player.set_looping') {
        this.handleSetLooping(call, result);
        return;
      }
      if (call.method === 'audio_player.set_volume') {
        this.handleSetVolume(call, result);
        return;
      }
      if (call.method === 'audio_player.get_duration') {
        this.handleGetDuration(call, result);
        return;
      }
      if (call.method === 'audio_player.get_position') {
        this.handleGetPosition(call, result);
        return;
      }
      if (call.method === 'audio_player.is_playing') {
        this.handleIsPlaying(call, result);
        return;
      }
      if (call.method === 'audio_player.listen') {
        this.handleListen(call, result);
        return;
      }
      if (call.method === 'audio_player.dispose') {
        this.handleDispose(call, result);
        return;
      }
      result.error(
        call.requestId,
        REQUEST_ERROR_PLAYER_NOT_FOUND,
        'method_not_implemented',
        false
      );
    } catch (err) {
      result.error(call.requestId, REQUEST_ERROR_INTERNAL, this.errorText(err), false);
    }
  }

  release(): void {
    const records = Array.from(this.players.values());
    this.players.clear();
    for (const record of records) {
      this.releaseRecord(record, REQUEST_ERROR_SOURCE_REPLACED, 'plugin_released');
    }
  }

  private handleSetSource(call: LeafMethodCall, result: LeafResult): void {
    const args = this.parseArgs(call.args);
    if (args.playerId.length === 0 || args.source.length === 0) {
      result.error(call.requestId, REQUEST_ERROR_INVALID_ARGS, 'invalid_args', false);
      return;
    }

    const record = this.getOrCreateRecord(args.playerId);
    this.resolveSource(record, false, 'source_replaced');
    this.releasePlayer(record, REQUEST_ERROR_SOURCE_REPLACED, 'source_replaced');
    record.source = args.source;
    record.events = [];
    record.pendingSourceResult = result;
    record.pendingSourceRequestId = call.requestId;
    record.generation = record.generation + 1;
    this.createPlayer(record, record.generation);
  }

  private async createPlayer(record: PlayerRecord, generation: number): Promise<void> {
    try {
      const player = await media.createAVPlayer();
      if (record.generation !== generation) {
        await player.release();
        return;
      }

      const file = fileIo.openSync(record.source, fileIo.OpenMode.READ_ONLY);
      const size = fileIo.statSync(file.fd).size;
      record.player = player;
      record.file = file;

      player.on('stateChange', (
        state: media.AVPlayerState,
        _reason: media.StateChangeReason
      ): void => {
        this.handleStateChange(record, generation, state);
      });
      player.on('error', (error: BusinessError): void => {
        this.handlePlayerError(record, generation, error);
      });

      player.fdSrc = {
        fd: file.fd,
        offset: 0,
        length: size
      };
    } catch (err) {
      if (record.generation === generation) {
        this.resolveSource(record, false, this.errorText(err));
        this.releasePlayer(record, REQUEST_ERROR_INTERNAL, this.errorText(err));
      }
    }
  }

  private handleStateChange(
    record: PlayerRecord,
    generation: number,
    state: media.AVPlayerState
  ): void {
    if (record.generation !== generation || !record.player) {
      return;
    }

    if (state === 'initialized') {
      record.player.prepare().catch((err: BusinessError): void => {
        this.handlePlayerError(record, generation, err);
      });
      return;
    }

    if (state === 'prepared') {
      try {
        record.player.loop = record.looping;
        record.player.setVolume(record.volume);
        this.resolveSource(record, true, '');
      } catch (err) {
        this.handlePlayerError(record, generation, err as BusinessError);
      }
      return;
    }

    if (state === 'completed' && !record.looping) {
      const duration = record.player.duration;
      this.enqueueEvent(record, 'complete', '', 0, duration / 1000.0);
    }
  }

  private handlePlayerError(
    record: PlayerRecord,
    generation: number,
    error: BusinessError
  ): void {
    if (record.generation !== generation) {
      return;
    }
    const message = this.errorText(error);
    this.resolveSource(record, false, message);
    this.enqueueEvent(record, 'error', message, this.errorCode(error), 0.0);
  }

  private handlePlay(call: LeafMethodCall, result: LeafResult): void {
    const record = this.requireRecord(call, result);
    if (!record || !record.player) {
      if (record) {
        result.error(call.requestId, REQUEST_ERROR_SOURCE_NOT_SET, 'source_not_set', false);
      }
      return;
    }
    record.player.play()
      .then((): void => {
        result.success(call.requestId, this.playerJson(record));
      })
      .catch((err: BusinessError): void => {
        result.error(call.requestId, this.errorCode(err), this.errorText(err), false);
      });
  }

  private handlePause(call: LeafMethodCall, result: LeafResult): void {
    const record = this.requireRecord(call, result);
    if (!record || !record.player) {
      if (record) {
        result.error(call.requestId, REQUEST_ERROR_SOURCE_NOT_SET, 'source_not_set', false);
      }
      return;
    }
    record.player.pause()
      .then((): void => {
        result.success(call.requestId, this.playerJson(record));
      })
      .catch((err: BusinessError): void => {
        result.error(call.requestId, this.errorCode(err), this.errorText(err), false);
      });
  }

  private handleStop(call: LeafMethodCall, result: LeafResult): void {
    const record = this.requireRecord(call, result);
    if (!record || !record.player) {
      if (record) {
        result.error(call.requestId, REQUEST_ERROR_SOURCE_NOT_SET, 'source_not_set', false);
      }
      return;
    }
    record.player.stop()
      .then((): void => {
        result.success(call.requestId, this.playerJson(record));
      })
      .catch((err: BusinessError): void => {
        result.error(call.requestId, this.errorCode(err), this.errorText(err), false);
      });
  }

  private handleSeek(call: LeafMethodCall, result: LeafResult): void {
    const args = this.parseArgs(call.args);
    const record = this.requireRecord(call, result, args.playerId);
    if (!record || !record.player) {
      if (record) {
        result.error(call.requestId, REQUEST_ERROR_SOURCE_NOT_SET, 'source_not_set', false);
      }
      return;
    }
    const positionMilliseconds = Math.max(0.0, args.position) * 1000.0;
    try {
      record.player.seek(positionMilliseconds);
      result.success(call.requestId, this.playerJson(record));
    } catch (err) {
      result.error(call.requestId, REQUEST_ERROR_INTERNAL, this.errorText(err), false);
    }
  }

  private handleSetLooping(call: LeafMethodCall, result: LeafResult): void {
    const args = this.parseArgs(call.args);
    const record = this.requireRecord(call, result, args.playerId);
    if (!record) {
      return;
    }
    record.looping = args.looping;
    if (record.player && record.player.state !== 'idle' && record.player.state !== 'initialized') {
      try {
        record.player.loop = args.looping;
      } catch (err) {
        result.error(call.requestId, REQUEST_ERROR_INTERNAL, this.errorText(err), false);
        return;
      }
    }
    result.success(call.requestId, this.playerJson(record));
  }

  private handleSetVolume(call: LeafMethodCall, result: LeafResult): void {
    const args = this.parseArgs(call.args);
    const record = this.requireRecord(call, result, args.playerId);
    if (!record) {
      return;
    }
    record.volume = Math.min(1.0, Math.max(0.0, args.volume));
    if (record.player) {
      try {
        record.player.setVolume(record.volume);
      } catch (err) {
        result.error(call.requestId, REQUEST_ERROR_INTERNAL, this.errorText(err), false);
        return;
      }
    }
    result.success(call.requestId, this.playerJson(record));
  }

  private handleGetDuration(call: LeafMethodCall, result: LeafResult): void {
    const record = this.requireRecord(call, result);
    if (!record) {
      return;
    }
    const payload = new PlayerPayload();
    payload.playerId = record.playerId;
    payload.duration = record.player && record.player.duration > 0
      ? record.player.duration / 1000.0
      : 0.0;
    result.success(call.requestId, JSON.stringify(payload));
  }

  private handleGetPosition(call: LeafMethodCall, result: LeafResult): void {
    const record = this.requireRecord(call, result);
    if (!record) {
      return;
    }
    const payload = new PlayerPayload();
    payload.playerId = record.playerId;
    payload.position = record.player && record.player.currentTime > 0
      ? record.player.currentTime / 1000.0
      : 0.0;
    result.success(call.requestId, JSON.stringify(payload));
  }

  private handleIsPlaying(call: LeafMethodCall, result: LeafResult): void {
    const record = this.requireRecord(call, result);
    if (!record) {
      return;
    }
    const payload = new PlayerPayload();
    payload.playerId = record.playerId;
    payload.playing = record.player !== null && record.player.state === 'playing';
    result.success(call.requestId, JSON.stringify(payload));
  }

  private handleListen(call: LeafMethodCall, result: LeafResult): void {
    const args = this.parseArgs(call.args);
    const record = this.requireRecord(call, result, args.playerId);
    if (!record) {
      return;
    }
    if (record.pendingListener !== null) {
      result.error(call.requestId, REQUEST_ERROR_LISTEN_IN_PROGRESS, 'listen_in_progress', false);
      return;
    }
    if (record.events.length > 0) {
      const event = record.events.shift();
      result.success(call.requestId, event ? event : '{}');
      return;
    }
    record.pendingListener = new PendingListener(call.requestId, result);
  }

  private handleDispose(call: LeafMethodCall, result: LeafResult): void {
    const args = this.parseArgs(call.args);
    const record = this.players.get(args.playerId);
    if (record) {
      this.players.delete(args.playerId);
      this.releaseRecord(record, REQUEST_ERROR_PLAYER_NOT_FOUND, 'player_released');
    }
    const payload = new PlayerPayload();
    payload.playerId = args.playerId;
    result.success(call.requestId, JSON.stringify(payload));
  }

  private requireRecord(
    call: LeafMethodCall,
    result: LeafResult,
    explicitPlayerId: string = ''
  ): PlayerRecord | null {
    const playerId = explicitPlayerId.length > 0
      ? explicitPlayerId
      : this.parseArgs(call.args).playerId;
    if (playerId.length === 0) {
      result.error(call.requestId, REQUEST_ERROR_INVALID_ARGS, 'invalid_args', false);
      return null;
    }
    const record = this.players.get(playerId);
    if (!record) {
      result.error(call.requestId, REQUEST_ERROR_PLAYER_NOT_FOUND, 'player_not_found', false);
      return null;
    }
    return record;
  }

  private getOrCreateRecord(playerId: string): PlayerRecord {
    const existing = this.players.get(playerId);
    if (existing) {
      return existing;
    }
    const record = new PlayerRecord(playerId);
    this.players.set(playerId, record);
    return record;
  }

  private releaseRecord(record: PlayerRecord, code: number, error: string): void {
    record.generation = record.generation + 1;
    this.resolveSource(record, false, error);
    if (record.pendingListener) {
      record.pendingListener.result.error(
        record.pendingListener.requestId,
        code,
        error,
        true
      );
      record.pendingListener = null;
    }
    this.releasePlayer(record, code, error);
  }

  private releasePlayer(record: PlayerRecord, _code: number, _error: string): void {
    const player = record.player;
    const file = record.file;
    record.player = null;
    record.file = null;
    record.events = [];

    if (!player) {
      this.closeFile(file);
      return;
    }

    try {
      player.off('stateChange');
      player.off('error');
    } catch (_err) {
    }
    player.release()
      .then((): void => {
        this.closeFile(file);
      })
      .catch((_err: BusinessError): void => {
        this.closeFile(file);
      });
  }

  private closeFile(file: fileIo.File | null): void {
    if (!file) {
      return;
    }
    try {
      fileIo.closeSync(file);
    } catch (_err) {
    }
  }

  private resolveSource(record: PlayerRecord, success: boolean, error: string): void {
    const pending = record.pendingSourceResult;
    const requestId = record.pendingSourceRequestId;
    record.pendingSourceResult = null;
    record.pendingSourceRequestId = 0;
    if (!pending) {
      return;
    }
    if (success) {
      pending.success(requestId, this.playerJson(record));
      return;
    }
    pending.error(
      requestId,
      REQUEST_ERROR_INTERNAL,
      error.length > 0 ? error : 'audio_source_failed',
      false
    );
  }

  private enqueueEvent(
    record: PlayerRecord,
    type: string,
    error: string,
    code: number,
    position: number
  ): void {
    const payload = new AudioEventPayload();
    payload.playerId = record.playerId;
    payload.type = type;
    payload.error = error;
    payload.code = code;
    payload.position = position;
    const json = JSON.stringify(payload);

    if (record.pendingListener) {
      const listener = record.pendingListener;
      record.pendingListener = null;
      listener.result.success(listener.requestId, json);
      return;
    }

    record.events.push(json);
    if (record.events.length > MAX_PENDING_EVENTS) {
      record.events.shift();
    }
  }

  private playerJson(record: PlayerRecord): string {
    const payload = new PlayerPayload();
    payload.playerId = record.playerId;
    return JSON.stringify(payload);
  }

  private parseArgs(args: string): PlayerArgs {
    const parsed = new PlayerArgs();
    if (!args || args.trim().length === 0) {
      return parsed;
    }
    const raw = JSON.parse(args) as Record<string, number | string | boolean | null>;
    const playerId = raw['playerId'];
    if (typeof playerId === 'string') {
      parsed.playerId = playerId;
    }
    const source = raw['source'];
    if (typeof source === 'string') {
      parsed.source = source;
    }
    const position = raw['position'];
    if (typeof position === 'number' && Number.isFinite(position)) {
      parsed.position = position;
    }
    const looping = raw['looping'];
    if (typeof looping === 'boolean') {
      parsed.looping = looping;
    }
    const volume = raw['volume'];
    if (typeof volume === 'number' && Number.isFinite(volume)) {
      parsed.volume = volume;
    }
    return parsed;
  }

  private errorText(error: Object): string {
    const value = error as BusinessError;
    if (value && value.message && value.message.length > 0) {
      return value.message;
    }
    const text = `${error}`;
    return text.length > 0 ? text : 'audio_operation_failed';
  }

  private errorCode(error: BusinessError): number {
    return error && Number.isFinite(error.code) ? error.code : REQUEST_ERROR_INTERNAL;
  }
}
