import { display } from '@kit.ArkUI';
import { initEngine, registerPluginDispatcher } from 'libleaf_ohos.so';
import { PluginDispatcher } from '../plugin/PluginDispatcher';

@Component
export struct LeafView {
  private xComponentId: string = 'leaf_xcomponent';
  private isEngineInitialized: boolean = false;
  private isPluginBridgeInitialized: boolean = false;

  build() {
    Stack() {
      XComponent({
        id: this.xComponentId,
        type: XComponentType.SURFACE, // 必须是 Surface 以获得 OpenGL 环境
        libraryname: 'leaf_ohos' // 对应 CMakeLists.txt 里的 target 名字
      })
        .onLoad((context: object) => {
          if (!this.isEngineInitialized) {
            const resMgr = getContext(this).resourceManager;
            let displayClass = display.getDefaultDisplaySync();
            let density = displayClass.densityPixels;

            // 初始化引擎
            initEngine(resMgr, density);

            if (!this.isPluginBridgeInitialized) {
              const dispatcher = PluginDispatcher.getInstance();
              registerPluginDispatcher((method: string, requestId: number, args: string) => {
                dispatcher.dispatch(method, requestId, args);
              });
              this.isPluginBridgeInitialized = true;
            }

            this.isEngineInitialized = true;
            console.log('[Leaf]: Engine initialized, render thread should be running.');
          }
        })
        .width('100%')
        .height('100%')
    }
  }
}
