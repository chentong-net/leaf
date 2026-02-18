package net.chentong.leaf.android.plugin;

import java.util.Objects;

public interface LeafPlugin {

    String pluginName();

    boolean canHandle(String method);

    void onMethodCall(LeafMethodCall call, Result result);

    final class LeafMethodCall {
        private final int requestId;
        private final String method;
        private final String args;

        public LeafMethodCall(int requestId, String method, String args) {
            this.requestId = requestId;
            this.method = method == null ? "" : method;
            this.args = args == null ? "" : args;
        }

        public int getRequestId() {
            return requestId;
        }

        public String getMethod() {
            return method;
        }

        public String getArgs() {
            return args;
        }
    }

    interface Result {
        void success(int requestId, String data);

        void error(int requestId, int code, String error, boolean canceled);
    }

    static String requireName(String name) {
        String pluginName = Objects.requireNonNull(name, "pluginName");
        if (pluginName.isEmpty()) {
            throw new IllegalArgumentException("pluginName is empty");
        }
        return pluginName;
    }
}
