export const initEngine: (resMgr: any, density: number) => void;
export const initFileServiceBridge: (
  requestHandler: (
    op: number,
    requestId: number,
    mediaType: number,
    copyToSandbox: boolean,
    fileId: string
  ) => void
) => boolean;
export const notifyPickResult: (
  requestId: number,
  success: boolean,
  canceled: boolean,
  fileId: string,
  name: string,
  path: string,
  mimeType: string,
  size: number,
  error: string
) => void;
export const notifyReadResult: (
  requestId: number,
  success: boolean,
  canceled: boolean,
  content: string,
  error: string
) => void;
