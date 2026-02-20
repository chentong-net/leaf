import { FilePickerPlugin } from 'file_picker'
import { PathProviderPlugin } from 'path_provider'
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
    const context = getContext(this) as Context;
    PluginRegistry.getInstance().register(new FilePickerPlugin(context));
    PluginRegistry.getInstance().register(new PathProviderPlugin(context));
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
