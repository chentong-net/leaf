import { Context } from '@kit.AbilityKit';
import { fileIo } from '@kit.CoreFileKit';
import { LeafMethodCall, LeafPlugin, LeafResult } from 'leaf_plugin';

class PathPayload {
  path: string = '';
}

const REQUEST_ERROR_METHOD_NOT_IMPLEMENTED = -404;
const REQUEST_ERROR_SERIALIZE_FAILED = -500;

export class PathProviderPlugin implements LeafPlugin {
  private readonly context: Context;

  constructor(context: Context) {
    this.context = context;
  }

  pluginName(): string {
    return 'PathProviderPlugin';
  }

  canHandle(method: string): boolean {
    return method === 'path_provider.get_temporary_path'
      || method === 'path_provider.get_application_support_path'
      || method === 'path_provider.get_application_documents_path'
      || method === 'path_provider.get_downloads_path'
      || method === 'path_provider.get_external_storage_path';
  }

  onMethodCall(call: LeafMethodCall, result: LeafResult): void {
    let path = '';
    if (call.method === 'path_provider.get_temporary_path') {
      path = this.context.cacheDir;
      this.replyPath(call, result, path);
      return;
    }
    if (call.method === 'path_provider.get_application_support_path') {
      path = `${this.context.filesDir}/leaf/app_support`;
      this.ensureDirectory(path);
      this.replyPath(call, result, path);
      return;
    }
    if (call.method === 'path_provider.get_application_documents_path') {
      path = `${this.context.filesDir}/leaf/documents`;
      this.ensureDirectory(path);
      this.replyPath(call, result, path);
      return;
    }
    if (call.method === 'path_provider.get_downloads_path') {
      this.replyPath(call, result, '');
      return;
    }
    if (call.method === 'path_provider.get_external_storage_path') {
      this.replyPath(call, result, '');
      return;
    }
    result.error(call.requestId, REQUEST_ERROR_METHOD_NOT_IMPLEMENTED, 'method_not_implemented', false);
  }

  private ensureDirectory(path: string): void {
    try {
      fileIo.mkdirSync(path, true);
    } catch (_err) {
    }
  }

  private replyPath(call: LeafMethodCall, result: LeafResult, path: string): void {
    const payload = new PathPayload();
    payload.path = path;
    try {
      result.success(call.requestId, JSON.stringify(payload));
    } catch (_err) {
      result.error(call.requestId, REQUEST_ERROR_SERIALIZE_FAILED, 'serialize_result_failed', false);
    }
  }
}
