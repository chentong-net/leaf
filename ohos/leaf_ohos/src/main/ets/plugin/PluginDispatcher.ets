import { notifyPluginResult } from 'libleaf_ohos.so';
import { LeafMethodCall, LeafResult, LeafPlugin } from 'leaf_plugin';
import { PluginRegistry } from './PluginRegistry';

export type ResultEmitter = (
  requestId: number,
  ok: boolean,
  code: number,
  canceled: boolean,
  data: string,
  error: string
) => void;

class DispatchMethodCall implements LeafMethodCall {
  requestId: number;
  method: string;
  args: string;

  constructor(requestId: number, method: string, args: string) {
    this.requestId = requestId;
    this.method = method;
    this.args = args;
  }
}

class DispatchResult implements LeafResult {
  private completed: boolean = false;
  private readonly requestId: number;
  private readonly successEmitter: (requestId: number, data: string) => void;
  private readonly errorEmitter: (requestId: number, code: number, error: string, canceled: boolean) => void;

  constructor(
    requestId: number,
    successEmitter: (requestId: number, data: string) => void,
    errorEmitter: (requestId: number, code: number, error: string, canceled: boolean) => void
  ) {
    this.requestId = requestId;
    this.successEmitter = successEmitter;
    this.errorEmitter = errorEmitter;
  }

  success(callbackRequestId: number, data: string): void {
    if (this.completed) {
      return;
    }
    this.completed = true;
    const finalRequestId = callbackRequestId > 0 ? callbackRequestId : this.requestId;
    this.successEmitter(finalRequestId, data);
  }

  error(callbackRequestId: number, code: number, error: string, canceled: boolean): void {
    if (this.completed) {
      return;
    }
    this.completed = true;
    const finalRequestId = callbackRequestId > 0 ? callbackRequestId : this.requestId;
    this.errorEmitter(finalRequestId, code, error, canceled);
  }

  isCompleted(): boolean {
    return this.completed;
  }
}

export class PluginDispatcher {
  private static readonly INSTANCE: PluginDispatcher = new PluginDispatcher(PluginRegistry.getInstance());

  private readonly registry: PluginRegistry;
  private resultEmitter: ResultEmitter;

  private constructor(registry: PluginRegistry) {
    this.registry = registry;
    this.resultEmitter = (
      requestId: number,
      ok: boolean,
      code: number,
      canceled: boolean,
      data: string,
      error: string
    ) => {
      notifyPluginResult(requestId, ok, code, canceled, data, error);
    };
  }

  static getInstance(): PluginDispatcher {
    return PluginDispatcher.INSTANCE;
  }

  getRegistry(): PluginRegistry {
    return this.registry;
  }

  setResultEmitter(emitter: ResultEmitter): void {
    this.resultEmitter = emitter;
  }

  dispatch(method: string, requestId: number, args: string): void {
    if (!method || method.length === 0) {
      this.emitError(requestId, -1, 'method_empty', false);
      return;
    }

    const plugin: LeafPlugin | null = this.registry.findByMethod(method);
    if (!plugin) {
      this.emitError(requestId, -404, 'method_not_implemented', false);
      return;
    }

    const result = new DispatchResult(
      requestId,
      (finalRequestId: number, data: string): void => {
        this.emitSuccess(finalRequestId, data);
      },
      (finalRequestId: number, code: number, error: string, canceled: boolean): void => {
        this.emitError(finalRequestId, code, error, canceled);
      }
    );

    try {
      const call: LeafMethodCall = new DispatchMethodCall(requestId, method, args ? args : '');
      plugin.onMethodCall(call, result);
    } catch (err) {
      if (result.isCompleted()) {
        return;
      }
      result.error(requestId, -500, this.toErrorString(err), false);
    }
  }

  private emitSuccess(requestId: number, data: string): void {
    if (!this.resultEmitter) {
      return;
    }
    this.resultEmitter(requestId, true, 0, false, data ? data : '', '');
  }

  private emitError(requestId: number, code: number, error: string, canceled: boolean): void {
    if (!this.resultEmitter) {
      return;
    }
    const errorText = error && error.length > 0 ? error : 'unknown_error';
    this.resultEmitter(requestId, false, code, canceled, '', errorText);
  }

  private toErrorString(err: Object): string {
    const text = `${err}`;
    if (text && text.length > 0) {
      return text;
    }
    return 'plugin_dispatch_throw';
  }
}
