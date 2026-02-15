#ifndef LEAF_LFPLUGIN_H
#define LEAF_LFPLUGIN_H

#include "LFDef.h"

struct LFPluginResult {
    bool ok = false;
    std::string data;
    std::string error;
    int code = 0;
};

using LFPluginCallback = std::function<void(const LFPluginResult&)>;
using LFPluginInvokeHandler = std::function<void(
        const std::string& method,
        const std::string& params,
        LFPluginCallback callback)>;

class LFPluginCenter {
public:
    static LFPluginCenter& getInstance();

    void registerPlugin(const std::string& pluginName, LFPluginInvokeHandler handler);
    void invoke(
            const std::string& pluginName,
            const std::string& method,
            const std::string& params,
            LFPluginCallback callback);

    static void dispatchToMain(std::function<void()> task);

private:
    LFPluginCenter() = default;

    std::unordered_map<std::string, LFPluginInvokeHandler> m_handlers;
    std::mutex m_mutex;
};

#endif // LEAF_LFPLUGIN_H
