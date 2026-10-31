import { i18n } from '@kit.LocalizationKit';
import { LeafMethodCall, LeafPlugin, LeafResult } from 'leaf_plugin';

const REQUEST_ERROR_INTERNAL = -500;
const REQUEST_ERROR_METHOD_NOT_IMPLEMENTED = -404;

class LanguagePayload {
  languageTag: string = '';
}

export class I18nPlugin implements LeafPlugin {
  pluginName(): string {
    return 'I18nPlugin';
  }

  canHandle(method: string): boolean {
    return method === 'i18n.get_system_language';
  }

  onMethodCall(call: LeafMethodCall, result: LeafResult): void {
    if (call.method !== 'i18n.get_system_language') {
      result.error(
        call.requestId,
        REQUEST_ERROR_METHOD_NOT_IMPLEMENTED,
        'method_not_implemented',
        false
      );
      return;
    }

    try {
      const language = i18n.System.getSystemLanguage();
      const region = i18n.System.getSystemRegion();
      const languageTag = region.length > 0
        ? `${language}-${region}`
        : language;
      if (languageTag.length === 0) {
        result.error(call.requestId, -2, 'system_language_empty', false);
        return;
      }

      const payload = new LanguagePayload();
      payload.languageTag = languageTag;
      result.success(call.requestId, JSON.stringify(payload));
    } catch (_err) {
      result.error(
        call.requestId,
        REQUEST_ERROR_INTERNAL,
        'get_system_language_failed',
        false
      );
    }
  }
}
