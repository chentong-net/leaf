export interface LeafMethodCall {
  requestId: number;
  method: string;
  args: string;
}

export interface LeafResult {
  success(requestId: number, data: string): void;
  error(requestId: number, code: number, error: string, canceled: boolean): void;
}

export interface LeafPlugin {
  pluginName(): string;
  canHandle(method: string): boolean;
  onMethodCall(call: LeafMethodCall, result: LeafResult): void;
}

export function requirePluginName(name: string): string {
  if (!name || name.length === 0) {
    throw new Error('pluginName is empty');
  }
  return name;
}
