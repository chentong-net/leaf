import { common } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import picker from '@ohos.file.picker';
import fileIo from '@ohos.file.fs';
import fileUri from '@ohos.file.fileuri';
import {
  initFileServiceBridge,
  notifyPickResult,
  notifyReadResult,
  notifySaveResult
} from 'libleaf_ohos.so';

const OP_PICK: number = 1;
const OP_READ: number = 2;
const OP_SAVE: number = 3;
const FILE_SERVICE_DOMAIN: number = 0x0000;
const FILE_SERVICE_TAG: string = 'LeafFileService';

class PickedRecord {
  fileId: string = '';
  uri: string = '';
  name: string = '';
  mimeType: string = '';
  size: number = 0;
  localPath: string = '';
}

class LeafFileServiceBridge {
  private static initialized: boolean = false;
  private static appContext: common.Context | null = null;
  private static records: Map<string, PickedRecord> = new Map<string, PickedRecord>();

  static initialize(context?: common.Context): void {
    if (context) {
      LeafFileServiceBridge.appContext = context;
    }

    if (LeafFileServiceBridge.initialized) {
      return;
    }

    const ok: boolean = initFileServiceBridge((
      op: number,
      requestId: number,
      mediaType: number,
      copyToSandbox: boolean,
      fileId: string,
      fileName: string,
      content: string
    ): void => {
      void LeafFileServiceBridge.handleRequest(
        op,
        requestId,
        mediaType,
        copyToSandbox,
        fileId,
        fileName,
        content
      );
    });

    if (ok) {
      LeafFileServiceBridge.initialized = true;
    }
  }

  private static async handleRequest(
    op: number,
    requestId: number,
    mediaType: number,
    copyToSandbox: boolean,
    fileId: string,
    fileName: string,
    content: string
  ): Promise<void> {
    if (op === OP_PICK) {
      await LeafFileServiceBridge.handlePick(requestId, mediaType, copyToSandbox);
      return;
    }

    if (op === OP_READ) {
      await LeafFileServiceBridge.handleRead(requestId, fileId);
      return;
    }

    if (op === OP_SAVE) {
      await LeafFileServiceBridge.handleSave(requestId, fileName, content);
      return;
    }

    notifyPickResult(requestId, false, false, '', '', '', '', 0, 'unknown_operation');
  }

  private static async handlePick(requestId: number, mediaType: number, copyToSandbox: boolean): Promise<void> {
    try {
      const viewPicker: picker.DocumentViewPicker = LeafFileServiceBridge.createPicker();
      const options: picker.DocumentSelectOptions = LeafFileServiceBridge.buildSelectOptions(mediaType);
      const uris: Array<string> = await viewPicker.select(options);

      if (uris.length === 0) {
        notifyPickResult(requestId, false, true, '', '', '', '', 0, 'canceled');
        return;
      }

      const selectedUri: string = uris[0];
      hilog.info(FILE_SERVICE_DOMAIN, FILE_SERVICE_TAG, 'picked uri: %{public}s', selectedUri);
      const record: PickedRecord = new PickedRecord();
      record.fileId = LeafFileServiceBridge.createFileId();
      record.uri = selectedUri;
      record.name = LeafFileServiceBridge.resolveName(selectedUri);
      record.mimeType = LeafFileServiceBridge.guessMimeType(record.name, mediaType);

      if (copyToSandbox) {
        record.localPath = await LeafFileServiceBridge.copyUriToSandbox(selectedUri, record.name);
        if (record.localPath.length === 0) {
          notifyPickResult(requestId, false, false, '', '', '', '', 0, 'copy_to_sandbox_failed');
          return;
        }
      } else {
        record.localPath = '';
      }

      record.size = await LeafFileServiceBridge.resolveSize(record.localPath);
      LeafFileServiceBridge.records.set(record.fileId, record);

      notifyPickResult(
        requestId,
        true,
        false,
        record.fileId,
        record.name,
        record.localPath,
        record.mimeType,
        record.size,
        ''
      );
    } catch (err) {
      const message: string = LeafFileServiceBridge.resolveErrorMessage(err);
      notifyPickResult(requestId, false, false, '', '', '', '', 0, message);
    }
  }

  private static async handleRead(requestId: number, fileId: string): Promise<void> {
    const record: PickedRecord | undefined = LeafFileServiceBridge.records.get(fileId);
    if (!record) {
      notifyReadResult(requestId, false, false, '', 'file_not_found');
      return;
    }

    try {
      let sourcePath: string = record.localPath;
      if (sourcePath.length === 0) {
        sourcePath = await LeafFileServiceBridge.copyUriToSandbox(record.uri, record.name);
      }
      if (sourcePath.length === 0) {
        notifyReadResult(requestId, false, false, '', 'read_source_unavailable');
        return;
      }

      const content: string = await fileIo.readText(sourcePath);
      notifyReadResult(requestId, true, false, content, '');
    } catch (_) {
      notifyReadResult(requestId, false, false, '', 'read_failed');
    }
  }

  private static async handleSave(requestId: number, fileName: string, content: string): Promise<void> {
    try {
      const viewPicker: picker.DocumentViewPicker = LeafFileServiceBridge.createPicker();
      const options: picker.DocumentSaveOptions = new picker.DocumentSaveOptions();
      const resolvedName: string = fileName.length > 0 ? fileName : 'leaf_output.txt';
      options.newFileNames = [resolvedName];

      const uris: Array<string> = await viewPicker.save(options);
      if (uris.length === 0) {
        notifySaveResult(requestId, false, true, '', 'canceled');
        return;
      }

      const targetUri: string = uris[0];
      const tempPath: string = await LeafFileServiceBridge.writeTempFile(resolvedName, content);
      if (tempPath.length === 0) {
        notifySaveResult(requestId, false, false, '', 'save_prepare_failed');
        return;
      }

      await fileIo.copyFile(tempPath, targetUri);
      notifySaveResult(requestId, true, false, targetUri, '');
    } catch (_) {
      notifySaveResult(requestId, false, false, '', 'save_failed');
    }
  }

  private static buildSelectOptions(mediaType: number): picker.DocumentSelectOptions {
    const options: picker.DocumentSelectOptions = new picker.DocumentSelectOptions();
    options.maxSelectNumber = 1;

    if (mediaType === 1) {
      options.fileSuffixFilters = [
        'Images|.jpg,.jpeg,.png,.gif,.bmp,.webp,.heic'
      ];
      return options;
    }

    if (mediaType === 2) {
      options.fileSuffixFilters = [
        'Videos|.mp4,.mov,.m4v,.avi,.mkv,.webm'
      ];
      return options;
    }

    if (mediaType === 3) {
      options.fileSuffixFilters = [
        'Images And Videos|.jpg,.jpeg,.png,.gif,.bmp,.webp,.heic,.mp4,.mov,.m4v,.avi,.mkv,.webm'
      ];
      return options;
    }

    return options;
  }

  private static createPicker(): picker.DocumentViewPicker {
    if (LeafFileServiceBridge.appContext) {
      return new picker.DocumentViewPicker(LeafFileServiceBridge.appContext);
    }
    return new picker.DocumentViewPicker();
  }

  private static createFileId(): string {
    const randomSuffix: string = Math.floor(Math.random() * 1000000).toString();
    return 'lf_file_' + Date.now().toString() + '_' + randomSuffix;
  }

  private static resolveName(uri: string): string {
    try {
      const parsed: fileUri.FileUri = new fileUri.FileUri(uri);
      if (parsed.name.length > 0) {
        return parsed.name;
      }
    } catch (_) {
    }

    const lastSlash: number = uri.lastIndexOf('/');
    if (lastSlash >= 0 && lastSlash < uri.length - 1) {
      return uri.substring(lastSlash + 1);
    }
    return 'picked_file';
  }

  private static guessMimeType(fileName: string, mediaType: number): string {
    const lowerName: string = fileName.toLowerCase();

    if (
      lowerName.endsWith('.jpg') ||
      lowerName.endsWith('.jpeg') ||
      lowerName.endsWith('.png') ||
      lowerName.endsWith('.gif') ||
      lowerName.endsWith('.bmp') ||
      lowerName.endsWith('.webp') ||
      lowerName.endsWith('.heic')
    ) {
      return 'image/*';
    }

    if (
      lowerName.endsWith('.mp4') ||
      lowerName.endsWith('.mov') ||
      lowerName.endsWith('.m4v') ||
      lowerName.endsWith('.avi') ||
      lowerName.endsWith('.mkv') ||
      lowerName.endsWith('.webm')
    ) {
      return 'video/*';
    }

    if (mediaType === 1) {
      return 'image/*';
    }
    if (mediaType === 2) {
      return 'video/*';
    }
    if (mediaType === 3) {
      return 'media/*';
    }
    return '*/*';
  }

  private static async resolveSize(path: string): Promise<number> {
    if (path.length === 0) {
      return 0;
    }
    try {
      const stat = await fileIo.stat(path);
      return Number(stat.size);
    } catch (_) {
      return 0;
    }
  }

  private static sanitizeName(rawName: string): string {
    if (rawName.length === 0) {
      return 'picked_file';
    }

    const safeName: string = rawName
      .replaceAll('\\', '_')
      .replaceAll('/', '_')
      .replaceAll(':', '_')
      .replaceAll('*', '_')
      .replaceAll('?', '_')
      .replaceAll('"', '_')
      .replaceAll('<', '_')
      .replaceAll('>', '_')
      .replaceAll('|', '_');

    return safeName.length > 0 ? safeName : 'picked_file';
  }

  private static ensureSandboxDir(): string {
    const ctx: common.Context | null = LeafFileServiceBridge.appContext;
    if (!ctx) {
      return '';
    }

    const rootDir: string = ctx.cacheDir;
    if (rootDir.length === 0) {
      return '';
    }

    const targetDir: string = rootDir + '/leaf';
    try {
      fileIo.mkdirSync(targetDir);
    } catch (e) {
      console.log("mkdirSync err: ", targetDir, e)
    }
    return targetDir;
  }

  private static async copyUriToSandbox(uri: string, preferredName: string): Promise<string> {
    const dir: string = LeafFileServiceBridge.ensureSandboxDir();
    if (dir.length === 0) {
      hilog.error(FILE_SERVICE_DOMAIN, FILE_SERVICE_TAG, 'copy failed: sandbox dir unavailable');
      return '';
    }

    const safeName: string = LeafFileServiceBridge.sanitizeName(preferredName);
    const outPath: string = dir + '/' + Date.now().toString() + '_' + safeName;

    if (LeafFileServiceBridge.copyByStream(uri, outPath)) {
      hilog.info(FILE_SERVICE_DOMAIN, FILE_SERVICE_TAG, 'copy success(stream): source=%{public}s', uri);
      return outPath;
    }

    hilog.error(FILE_SERVICE_DOMAIN, FILE_SERVICE_TAG, 'copy failed: uri=%{public}s, out=%{public}s', uri, outPath);
    return '';
  }

  private static copyByStream(sourceUriOrPath: string, outPath: string): boolean {
    let srcFd: number = -1;
    let dstFd: number = -1;
    try {
      const srcFile = fileIo.openSync(sourceUriOrPath, fileIo.OpenMode.READ_ONLY);
      srcFd = srcFile.fd;
      const dstFile = fileIo.openSync(
        outPath,
        fileIo.OpenMode.READ_WRITE | fileIo.OpenMode.CREATE
      );
      dstFd = dstFile.fd;

      const bufferSize: number = 64 * 1024;
      const buffer: ArrayBuffer = new ArrayBuffer(bufferSize);
      while (true) {
        const readLen: number = fileIo.readSync(srcFd, buffer);
        if (readLen <= 0) {
          break;
        }

        let writeOffset: number = 0;
        while (writeOffset < readLen) {
          const written: number = fileIo.writeSync(dstFd, buffer, {
            offset: writeOffset,
            length: readLen - writeOffset
          });
          if (written <= 0) {
            throw new Error('write_zero');
          }
          writeOffset += written;
        }
      }

      fileIo.closeSync(dstFd);
      dstFd = -1;
      fileIo.closeSync(srcFd);
      srcFd = -1;
      return true;
    } catch (err) {
      const message: string = LeafFileServiceBridge.resolveErrorMessage(err);
      hilog.warn(
        FILE_SERVICE_DOMAIN,
        FILE_SERVICE_TAG,
        'stream copy exception: source=%{public}s, message=%{public}s',
        sourceUriOrPath,
        message
      );
      try {
        if (dstFd >= 0) {
          fileIo.closeSync(dstFd);
        }
      } catch (_) {
      }
      try {
        if (srcFd >= 0) {
          fileIo.closeSync(srcFd);
        }
      } catch (_) {
      }
      try {
        fileIo.unlinkSync(outPath);
      } catch (_) {
      }
      return false;
    }
  }

  private static async writeTempFile(fileName: string, content: string): Promise<string> {
    const dir: string = LeafFileServiceBridge.ensureSandboxDir();
    if (dir.length === 0) {
      return '';
    }

    const safeName: string = LeafFileServiceBridge.sanitizeName(fileName);
    const path: string = dir + '/save_' + Date.now().toString() + '_' + safeName;

    try {
      const raf = await fileIo.createRandomAccessFile(
        path,
        fileIo.OpenMode.READ_WRITE | fileIo.OpenMode.CREATE | fileIo.OpenMode.TRUNC
      );
      await raf.write(content);
      raf.close();
      return path;
    } catch (_) {
      return '';
    }
  }

  private static resolveErrorMessage(err: Object): string {
    try {
      const obj: Record<string, Object> = err as Record<string, Object>;
      const msgObj: Object | undefined = obj['message'];
      if (msgObj) {
        return String(msgObj);
      }
    } catch (_) {
    }
    return 'unknown';
  }
}

export function ensureLeafFileServiceInitialized(context?: common.Context): void {
  LeafFileServiceBridge.initialize(context);
}
