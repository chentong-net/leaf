#include "plugin/LFPlugin.h"
#include "LFEngine.h"

LFPluginCenter& LFPluginCenter::getInstance() {
    static LFPluginCenter center;
    return center;
}

void LFPluginCenter::registerPlugin(const std::string& pluginName, LFPluginInvokeHandler handler) {
    if (pluginName.empty() || !handler) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[pluginName] = std::move(handler);
}

void LFPluginCenter::invoke(
        const std::string& pluginName,
        const std::string& method,
        const std::string& params,
        LFPluginCallback callback) {
    LFPluginInvokeHandler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(pluginName);
        if (it != m_handlers.end()) {
            handler = it->second;
        }
    }

    if (!handler) {
        if (callback) {
            LFPluginResult result;
            result.ok = false;
            result.error = "plugin_not_registered";
            result.code = -1;
            dispatchToMain([callback, result]() {
                callback(result);
            });
        }
        return;
    }

    handler(method, params, std::move(callback));
}

void LFPluginCenter::dispatchToMain(std::function<void()> task) {
    if (!task) {
        return;
    }
    LFEngine::getInstance().addFrameTask([task = std::move(task)]() mutable {
        task();
        return false;
    });
}
