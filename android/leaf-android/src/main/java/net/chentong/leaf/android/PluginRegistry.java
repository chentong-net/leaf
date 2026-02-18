package net.chentong.leaf.android;

import net.chentong.leaf.android.plugin.LeafPlugin;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public final class PluginRegistry {
    private static final PluginRegistry INSTANCE = new PluginRegistry();

    private final Map<String, LeafPlugin> pluginMap = new LinkedHashMap<>();

    private PluginRegistry() {
    }

    public static PluginRegistry getInstance() {
        return INSTANCE;
    }

    public synchronized void register(LeafPlugin plugin) {
        LeafPlugin pluginImpl = Objects.requireNonNull(plugin, "plugin");
        String pluginName = LeafPlugin.requireName(pluginImpl.pluginName());
        pluginMap.put(pluginName, pluginImpl);
    }

    public synchronized void unregister(String pluginName) {
        if (pluginName == null || pluginName.isEmpty()) {
            return;
        }
        pluginMap.remove(pluginName);
    }

    public synchronized void clear() {
        pluginMap.clear();
    }

    public synchronized LeafPlugin findByMethod(String method) {
        if (method == null || method.isEmpty()) {
            return null;
        }
        for (LeafPlugin plugin : pluginMap.values()) {
            try {
                if (plugin.canHandle(method)) {
                    return plugin;
                }
            } catch (Throwable ignored) {
                // 插件异常不应影响其他插件匹配流程
            }
        }
        return null;
    }

    public synchronized List<String> pluginNames() {
        return new ArrayList<>(pluginMap.keySet());
    }
}
