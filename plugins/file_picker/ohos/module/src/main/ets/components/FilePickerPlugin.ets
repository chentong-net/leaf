import { Context } from '@kit.AbilityKit';
import { fileIo, picker } from '@kit.CoreFileKit';
import { LeafMethodCall, LeafPlugin, LeafResult } from 'leaf_plugin';

class PickOptions {
  mediaType: number = 0;
  copyToSandbox: boolean = true;
}

class PendingPick {
  requestId: number = 0;
  copyToSandbox: boolean = true;
  result: LeafResult;

  constructor(requestId: number, copyToSandbox: boolean, result: LeafResult) {
    this.requestId = requestId;
    this.copyToSandbox = copyToSandbox;
    this.result = result;
  }
}

class PickedFileRecord {
  fileId: string = '';
  uri: string = '';
  name: string = '';
  mimeType: string = '';
  path: string = '';
  size: number = 0;
}

class OpenFdArgs {
  fileId: string = '';
}

const REQUEST_ERROR_INVALID_ARGS = -1;
const REQUEST_ERROR_PICK_FAILED = -2;
const REQUEST_ERROR_CONTEXT_UNAVAILABLE = -10;
const REQUEST_ERROR_PICK_IN_PROGRESS = -11;
const REQUEST_ERROR_OPEN_PICKER_FAILED = -12;
const REQUEST_ERROR_READ_SOURCE_UNAVAILABLE = -20;
const REQUEST_ERROR_FILE_NOT_FOUND = -404;

export class FilePickerPlugin implements LeafPlugin {
  private readonly context: Context;
  private readonly pickedFiles: Map<string, PickedFileRecord> = new Map<string, PickedFileRecord>();
  private pendingPick: PendingPick | null = null;
  private nextFileId: number = 1;

  constructor(context: Context) {
    this.context = context;
  }

  pluginName(): string {
    return 'FilePickerPlugin';
  }

  canHandle(method: string): boolean {
    return method === 'file_picker.pick' || method === 'file_picker.open_fd';
  }

  onMethodCall(call: LeafMethodCall, result: LeafResult): void {
    if (call.method === 'file_picker.pick') {
      this.handlePick(call, result);
      return;
    }
    if (call.method === 'file_picker.open_fd') {
      this.handleOpenFd(call, result);
      return;
    }
    result.error(call.requestId, REQUEST_ERROR_FILE_NOT_FOUND, 'method_not_implemented', false);
  }

  private handlePick(call: LeafMethodCall, result: LeafResult): void {
    if (!this.context) {
      result.error(call.requestId, REQUEST_ERROR_CONTEXT_UNAVAILABLE, 'context_unavailable', false);
      return;
    }
    if (this.pendingPick !== null) {
      result.error(call.requestId, REQUEST_ERROR_PICK_IN_PROGRESS, 'pick_in_progress', false);
      return;
    }

    let options: PickOptions;
    try {
      options = this.parsePickOptions(call.args);
    } catch (_err) {
      result.error(call.requestId, REQUEST_ERROR_INVALID_ARGS, 'invalid_args', false);
      return;
    }

    const pending = new PendingPick(call.requestId, options.copyToSandbox, result);
    this.pendingPick = pending;
    this.openPicker(options);
  }

  private async openPicker(options: PickOptions): Promise<void> {
    try {
      const pickerInstance = new picker.DocumentViewPicker(this.context);
      const selectOptions = this.buildSelectOptions(options.mediaType);
      const uris = await pickerInstance.select(selectOptions);
      const current = this.pendingPick;
      this.pendingPick = null;
      if (current === null) {
        return;
      }
      if (!uris || uris.length === 0) {
        current.result.error(current.requestId, 0, 'canceled', true);
        return;
      }
      const uri = uris[0];
      this.resolvePickedUri(uri, current);
    } catch (err) {
      const current = this.pendingPick;
      this.pendingPick = null;
      if (current === null) {
        return;
      }
      if (this.isCanceled(err)) {
        current.result.error(current.requestId, 0, 'canceled', true);
        return;
      }
      current.result.error(current.requestId, REQUEST_ERROR_OPEN_PICKER_FAILED, 'open_picker_failed', false);
    }
  }

  private resolvePickedUri(uri: string, pending: PendingPick): void {
    try {
      const fileId = this.nextFileIdText();
      const name = this.deriveName(uri);
      const mimeType = this.deriveMimeType(name);
      const sandboxPath = pending.copyToSandbox ? this.copyToSandbox(uri, name) : '';
      const fileSize = this.querySize(sandboxPath, uri);

      const record = new PickedFileRecord();
      record.fileId = fileId;
      record.uri = uri;
      record.name = name;
      record.mimeType = mimeType;
      record.path = sandboxPath;
      record.size = fileSize;
      this.pickedFiles.set(fileId, record);

      pending.result.success(pending.requestId, this.buildPickResultJson(record));
    } catch (_err) {
      pending.result.error(pending.requestId, REQUEST_ERROR_PICK_FAILED, 'pick_failed', false);
    }
  }

  private handleOpenFd(call: LeafMethodCall, result: LeafResult): void {
    let args: OpenFdArgs;
    try {
      args = this.parseOpenFdArgs(call.args);
    } catch (_err) {
      result.error(call.requestId, REQUEST_ERROR_INVALID_ARGS, 'invalid_args', false);
      return;
    }

    if (args.fileId.length === 0) {
      result.error(call.requestId, REQUEST_ERROR_INVALID_ARGS, 'invalid_args', false);
      return;
    }

    const record = this.pickedFiles.get(args.fileId);
    if (!record) {
      result.error(call.requestId, REQUEST_ERROR_FILE_NOT_FOUND, 'file_not_found', false);
      return;
    }

    const openTarget = record.path.length > 0 ? record.path : record.uri;
    if (openTarget.length === 0) {
      result.error(call.requestId, REQUEST_ERROR_READ_SOURCE_UNAVAILABLE, 'read_source_unavailable', false);
      return;
    }

    try {
      const file = this.openFileForRead(openTarget);
      const fd = file.fd;
      result.success(call.requestId, this.buildOpenFdResultJson(record.fileId, fd, record.path));
    } catch (_err) {
      result.error(call.requestId, REQUEST_ERROR_READ_SOURCE_UNAVAILABLE, 'open_fd_failed', false);
    }
  }

  private parsePickOptions(args: string): PickOptions {
    const options = new PickOptions();
    if (!args || args.trim().length === 0) {
      return options;
    }
    const raw = JSON.parse(args) as Record<string, number | string | boolean | null>;
    const mediaValue = raw['mediaType'];
    if (typeof mediaValue === 'number' && Number.isFinite(mediaValue)) {
      options.mediaType = mediaValue;
    }
    const copyValue = raw['copyToSandbox'];
    if (typeof copyValue === 'boolean') {
      options.copyToSandbox = copyValue;
    }
    return options;
  }

  private parseOpenFdArgs(args: string): OpenFdArgs {
    const parsed = new OpenFdArgs();
    if (!args || args.trim().length === 0) {
      return parsed;
    }
    const raw = JSON.parse(args) as Record<string, number | string | boolean | null>;
    const fileIdValue = raw['fileId'];
    if (typeof fileIdValue === 'string') {
      parsed.fileId = fileIdValue;
    }
    return parsed;
  }

  private buildSelectOptions(mediaType: number): picker.DocumentSelectOptions {
    const options = new picker.DocumentSelectOptions();
    options.maxSelectNumber = 1;
    if (mediaType === 1) {
      options.fileSuffixFilters = ['图片|.jpg,.jpeg,.png,.gif,.bmp,.webp,.heic,.heif'];
      return options;
    }
    if (mediaType === 2) {
      options.fileSuffixFilters = ['视频|.mp4,.mov,.mkv,.avi,.m4v,.3gp,.webm'];
      return options;
    }
    if (mediaType === 3) {
      options.fileSuffixFilters = [
        '图片|.jpg,.jpeg,.png,.gif,.bmp,.webp,.heic,.heif',
        '视频|.mp4,.mov,.mkv,.avi,.m4v,.3gp,.webm'
      ];
      return options;
    }
    return options;
  }

  private nextFileIdText(): string {
    const id = this.nextFileId;
    this.nextFileId = this.nextFileId + 1;
    return `fp_${id}`;
  }

  private copyToSandbox(sourceUri: string, displayName: string): string {
    const targetDir = `${this.context.cacheDir}/leaf/file_picker`;
    this.ensureDirectory(targetDir);

    const safeName = this.sanitizeFileName(displayName);
    const finalName = safeName.length > 0 ? safeName : 'picked_file';
    const targetPath = `${targetDir}/${Date.now()}_${finalName}`;
    this.copyByStream(sourceUri, targetPath);
    return targetPath;
  }

  private ensureDirectory(path: string): void {
    try {
      fileIo.mkdirSync(path, true);
    } catch (_err) {
    }
  }

  private copyByStream(sourcePathOrUri: string, targetPath: string): void {
    const srcFile = this.openFileForRead(sourcePathOrUri);
    const dstMode = fileIo.OpenMode.READ_WRITE | fileIo.OpenMode.CREATE | fileIo.OpenMode.TRUNC;
    const dstFile = fileIo.openSync(targetPath, dstMode);

    const buffer = new ArrayBuffer(8192);
    try {
      while (true) {
        const readBytes = fileIo.readSync(srcFile.fd, buffer);
        if (readBytes <= 0) {
          break;
        }
        fileIo.writeSync(dstFile.fd, buffer, { length: readBytes });
      }
    } finally {
      fileIo.closeSync(srcFile);
      fileIo.closeSync(dstFile);
    }
  }

  private querySize(path: string, uri: string): number {
    if (path.length > 0) {
      try {
        return fileIo.statSync(path).size;
      } catch (_err) {
      }
    }
    if (uri.length > 0) {
      try {
        return fileIo.statSync(uri).size;
      } catch (_err) {
      }
    }
    return 0;
  }

  private deriveName(uri: string): string {
    const cleanUri = this.removeQuery(uri);
    const lastIndex = cleanUri.lastIndexOf('/');
    if (lastIndex < 0 || lastIndex + 1 >= cleanUri.length) {
      return 'picked_file';
    }
    const encoded = cleanUri.substring(lastIndex + 1);
    if (encoded.length === 0) {
      return 'picked_file';
    }
    try {
      return decodeURIComponent(encoded);
    } catch (_err) {
      return encoded;
    }
  }

  private deriveMimeType(name: string): string {
    const lowerName = name.toLowerCase();
    if (lowerName.endsWith('.jpg') || lowerName.endsWith('.jpeg')) return 'image/jpeg';
    if (lowerName.endsWith('.png')) return 'image/png';
    if (lowerName.endsWith('.gif')) return 'image/gif';
    if (lowerName.endsWith('.webp')) return 'image/webp';
    if (lowerName.endsWith('.bmp')) return 'image/bmp';
    if (lowerName.endsWith('.heic')) return 'image/heic';
    if (lowerName.endsWith('.heif')) return 'image/heif';
    if (lowerName.endsWith('.mp4')) return 'video/mp4';
    if (lowerName.endsWith('.mov')) return 'video/quicktime';
    if (lowerName.endsWith('.mkv')) return 'video/x-matroska';
    if (lowerName.endsWith('.avi')) return 'video/x-msvideo';
    if (lowerName.endsWith('.m4v')) return 'video/x-m4v';
    if (lowerName.endsWith('.3gp')) return 'video/3gpp';
    if (lowerName.endsWith('.webm')) return 'video/webm';
    return '';
  }

  private removeQuery(uri: string): string {
    const index = uri.indexOf('?');
    if (index < 0) {
      return uri;
    }
    return uri.substring(0, index);
  }

  private sanitizeFileName(name: string): string {
    if (!name) {
      return '';
    }
    return name.replace(/[\\/:*?"<>|]/g, '_');
  }

  private openFileForRead(sourcePathOrUri: string): fileIo.File {
    try {
      return fileIo.openSync(sourcePathOrUri, fileIo.OpenMode.READ_ONLY);
    } catch (_err) {
    }
    throw new Error('open_read_failed');
  }

  private escapeJson(text: string): string {
    return text
      .replace(/\\/g, '\\\\')
      .replace(/"/g, '\\"')
      .replace(/\n/g, '\\n')
      .replace(/\r/g, '\\r')
      .replace(/\t/g, '\\t');
  }

  private buildPickResultJson(record: PickedFileRecord): string {
    return `{"fileId":"${this.escapeJson(record.fileId)}","path":"${this.escapeJson(record.path)}","name":"${this.escapeJson(record.name)}","mimeType":"${this.escapeJson(record.mimeType)}","size":${record.size}}`;
  }

  private buildOpenFdResultJson(fileId: string, fd: number, path: string): string {
    return `{"fileId":"${this.escapeJson(fileId)}","fd":${fd},"path":"${this.escapeJson(path)}"}`;
  }

  private isCanceled(err: Object): boolean {
    const text = `${err}`.toLowerCase();
    if (text.indexOf('cancel') >= 0) {
      return true;
    }
    return false;
  }
}
