package net.chentong.leaf.android;

import net.chentong.leaf.android.plugin.LeafPlugin;

import java.util.concurrent.atomic.AtomicBoolean;

public final class PluginDispatcher {
    public interface ResultEmitter {
        void emit(int requestId, boolean ok, int code, boolean canceled, String data, String error);
    }

    private static final PluginDispatcher INSTANCE = new PluginDispatcher(PluginRegistry.getInstance());

    private final PluginRegistry registry;
    private volatile ResultEmitter resultEmitter;

    private PluginDispatcher(PluginRegistry registry) {
        this.registry = registry;
        this.resultEmitter = (requestId, ok, code, canceled, data, error) -> {
            try {
                nativeNotifyPluginResult(requestId, ok, code, canceled, data, error);
            } catch (UnsatisfiedLinkError ignored) {
                // Native lib may not be loaded yet.
            }
        };
    }

    public static PluginDispatcher getInstance() {
        return INSTANCE;
    }

    public PluginRegistry getRegistry() {
        return registry;
    }

    public void setResultEmitter(ResultEmitter emitter) {
        this.resultEmitter = emitter;
    }

    public void dispatch(String method, int requestId, String args) {
        if (method == null || method.isEmpty()) {
            emitError(requestId, -1, "method_empty", false);
            return;
        }

        LeafPlugin plugin = registry.findByMethod(method);
        if (plugin == null) {
            emitError(requestId, -404, "method_not_implemented", false);
            return;
        }

        AtomicBoolean completed = new AtomicBoolean(false);
        LeafPlugin.Result result = new LeafPlugin.Result() {
            @Override
            public void success(int callbackRequestId, String data) {
                if (!completed.compareAndSet(false, true)) {
                    return;
                }
                int finalRequestId = callbackRequestId > 0 ? callbackRequestId : requestId;
                emitSuccess(finalRequestId, data);
            }

            @Override
            public void error(int callbackRequestId, int code, String error, boolean canceled) {
                if (!completed.compareAndSet(false, true)) {
                    return;
                }
                int finalRequestId = callbackRequestId > 0 ? callbackRequestId : requestId;
                emitError(finalRequestId, code, error, canceled);
            }
        };

        try {
            plugin.onMethodCall(new LeafPlugin.LeafMethodCall(requestId, method, args), result);
        } catch (Throwable throwable) {
            if (!completed.compareAndSet(false, true)) {
                return;
            }
            emitError(requestId, -500, throwable.getMessage(), false);
        }
    }

    private static native void nativeNotifyPluginResult(
        int requestId,
        boolean ok,
        int code,
        boolean canceled,
        String data,
        String error
    );

    private void emitSuccess(int requestId, String data) {
        ResultEmitter emitter = resultEmitter;
        if (emitter == null) {
            return;
        }
        emitter.emit(requestId, true, 0, false, data == null ? "" : data, "");
    }

    private void emitError(int requestId, int code, String error, boolean canceled) {
        ResultEmitter emitter = resultEmitter;
        if (emitter == null) {
            return;
        }
        String errorText = (error == null || error.isEmpty()) ? "unknown_error" : error;
        emitter.emit(requestId, false, code, canceled, "", errorText);
    }
}
