import { FilePickerPlugin } from 'file_picker'
import { PathProviderPlugin } from 'path_provider'
import { AudioPlayerPlugin } from 'audio_player'
import { I18nPlugin } from 'i18n'
import { Context } from '@kit.AbilityKit'
import { LeafView, PluginRegistry } from 'leaf_ohos'

@Entry
@Component
struct Index {
  private pluginRegistered: boolean = false;

  aboutToAppear(): void {
    if (this.pluginRegistered) {
      return;
    }
    const context = this.getUIContext().getHostContext() as Context;
    if (!context) {
      return;
    }
    PluginRegistry.getInstance().register(new FilePickerPlugin(context));
    PluginRegistry.getInstance().register(new PathProviderPlugin(context));
    PluginRegistry.getInstance().register(new AudioPlayerPlugin());
    PluginRegistry.getInstance().register(new I18nPlugin());
    this.pluginRegistered = true;
  }

  build() {
    RelativeContainer() {
      LeafView()
        .height('100%')
        .width('100%')
    }
    .height('100%')
    .width('100%')
  }
}
