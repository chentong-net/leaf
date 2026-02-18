import { LeafPlugin, requirePluginName } from 'leaf_plugin';

export class PluginRegistry {
  private static readonly INSTANCE: PluginRegistry = new PluginRegistry();
  private readonly pluginMap: Map<string, LeafPlugin> = new Map<string, LeafPlugin>();

  private constructor() {
  }

  static getInstance(): PluginRegistry {
    return PluginRegistry.INSTANCE;
  }

  register(plugin: LeafPlugin): void {
    const pluginName = requirePluginName(plugin.pluginName());
    this.pluginMap.set(pluginName, plugin);
  }

  unregister(pluginName: string): void {
    if (!pluginName || pluginName.length === 0) {
      return;
    }
    this.pluginMap.delete(pluginName);
  }

  clear(): void {
    this.pluginMap.clear();
  }

  findByMethod(method: string): LeafPlugin | null {
    if (!method || method.length === 0) {
      return null;
    }
    for (const plugin of this.pluginMap.values()) {
      try {
        if (plugin.canHandle(method)) {
          return plugin;
        }
      } catch (_err) {
      }
    }
    return null;
  }

  pluginNames(): string[] {
    return Array.from(this.pluginMap.keys());
  }
}
