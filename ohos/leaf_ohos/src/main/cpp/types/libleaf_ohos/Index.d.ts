export const initEngine: (resMgr: any, density: number) => void;
export const registerPluginDispatcher: (
  callback: (method: string, requestId: number, args: string) => void
) => void;
export const notifyPluginResult: (
  requestId: number,
  ok: boolean,
  code: number,
  canceled: boolean,
  data: string,
  error: string
) => void;
