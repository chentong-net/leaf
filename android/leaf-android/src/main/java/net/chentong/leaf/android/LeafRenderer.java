package net.chentong.leaf.android;

import android.content.res.AssetManager;
import android.content.res.Resources;
import android.opengl.GLSurfaceView;
import android.view.KeyEvent;
import android.view.MotionEvent;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import java.util.LinkedList;
import java.util.Queue;

public class LeafRenderer implements GLSurfaceView.Renderer {

    public interface TextInputFocusListener {
        void onTextInputFocusChanged(boolean focused);
    }

    private AssetManager assetManager;
    private final TextInputFocusListener textInputFocusListener;
    private boolean lastTextInputFocused = false;

    // Touch event queue (thread-safe)
    private final Queue<MotionEvent> touchEventQueue = new LinkedList<>();
    private final Queue<NativeKeyEvent> keyEventQueue = new LinkedList<>();
    private final Queue<Integer> charInputQueue = new LinkedList<>();
    private final Object queueLock = new Object();

    private static class NativeKeyEvent {
        int type;
        int keyCode;
        int modifiers;
        boolean repeat;
    }

    private static final int KEY_EVENT_DOWN = 0;
    private static final int KEY_EVENT_UP = 1;

    private static final int LF_KEY_UNKNOWN = 0;
    private static final int LF_KEY_ENTER = 13;
    private static final int LF_KEY_TAB = 9;
    private static final int LF_KEY_BACKSPACE = 8;
    private static final int LF_KEY_ESCAPE = 27;
    private static final int LF_KEY_DELETE = 127;
    private static final int LF_KEY_LEFT = 1001;
    private static final int LF_KEY_RIGHT = 1002;
    private static final int LF_KEY_UP = 1003;
    private static final int LF_KEY_DOWN = 1004;
    private static final int LF_KEY_HOME = 1005;
    private static final int LF_KEY_END = 1006;

    private static final int LF_MOD_NONE = 0;
    private static final int LF_MOD_SHIFT = 1 << 0;
    private static final int LF_MOD_CTRL = 1 << 1;
    private static final int LF_MOD_ALT = 1 << 2;
    private static final int LF_MOD_SUPER = 1 << 3;

    public LeafRenderer(AssetManager assetManager, TextInputFocusListener listener) {
        this.assetManager = assetManager;
        this.textInputFocusListener = listener;
    }

    // --- Native 方法声明 ---
    // 加载编译好的 libleaf-android.so
    static {
        System.loadLibrary("leaf-android");
    }

    // 对应 engine_jni.cpp 中的 extern "C" 函数
    private native void nativeOnSurfaceCreated(AssetManager assetManager);
    private native void nativeOnSurfaceChanged(int width, int height, float density);
    private native void nativeOnDrawFrame();
    private native boolean nativeIsTextInputFocused();

    // New: Touch event dispatch to native
    private native void nativeDispatchTouchEvent(
        int action,
        int actionIndex,
        int pointerCount,
        int[] pointerIds,
        float[] x,
        float[] y,
        float[] pressure,
        long eventTime
    );

    private native void nativeDispatchKeyEvent(
        int type,
        int keyCode,
        int modifiers,
        boolean repeat
    );

    private native void nativeDispatchCharInput(int codepoint);

    // --- 实现 GLSurfaceView.Renderer 接口 ---

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        // 传递 AssetManager 供 C++ 读取 main.js
        nativeOnSurfaceCreated(assetManager);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // 获取设备像素密度 (density)，用于 NanoVG 缩放绘制
        float density = Resources.getSystem().getDisplayMetrics().density;
        nativeOnSurfaceChanged(width, height, density);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        // 1. Process input events
        processTouchEvents();
        processKeyEvents();
        processCharInput();

        // 2. Render frame
        nativeOnDrawFrame();

        // 3. Sync text input focus state from native
        boolean focused = nativeIsTextInputFocused();
        if (focused != lastTextInputFocused) {
            lastTextInputFocused = focused;
            if (textInputFocusListener != null) {
                textInputFocusListener.onTextInputFocusChanged(focused);
            }
        }
    }

    /**
     * Queue touch event from main thread
     */
    public void queueTouchEvent(MotionEvent event) {
        // Copy event (MotionEvent will be recycled by system)
        MotionEvent eventCopy = MotionEvent.obtain(event);
        synchronized (queueLock) {
            touchEventQueue.offer(eventCopy);
        }
    }

    public void queueKeyEvent(int type, int keyCode, int modifiers, boolean repeat) {
        NativeKeyEvent event = new NativeKeyEvent();
        event.type = type;
        event.keyCode = keyCode;
        event.modifiers = modifiers;
        event.repeat = repeat;
        synchronized (queueLock) {
            keyEventQueue.offer(event);
        }
    }

    public void queueCharInput(int codepoint) {
        synchronized (queueLock) {
            charInputQueue.offer(codepoint);
        }
    }

    /**
     * Process touch events on GL thread
     */
    private void processTouchEvents() {
        while (true) {
            MotionEvent event = null;

            synchronized (queueLock) {
                if (touchEventQueue.isEmpty()) {
                    break;
                }
                event = touchEventQueue.poll();
            }

            if (event != null) {
                dispatchTouchEventToNative(event);
                event.recycle();  // Release event
            }
        }
    }

    private void processKeyEvents() {
        while (true) {
            NativeKeyEvent event = null;

            synchronized (queueLock) {
                if (keyEventQueue.isEmpty()) {
                    break;
                }
                event = keyEventQueue.poll();
            }

            if (event != null) {
                nativeDispatchKeyEvent(event.type, event.keyCode, event.modifiers, event.repeat);
            }
        }
    }

    private void processCharInput() {
        while (true) {
            Integer codepoint = null;
            synchronized (queueLock) {
                if (charInputQueue.isEmpty()) {
                    break;
                }
                codepoint = charInputQueue.poll();
            }

            if (codepoint != null && codepoint > 0) {
                nativeDispatchCharInput(codepoint);
            }
        }
    }

    /**
     * Convert MotionEvent to C++ format and dispatch
     */
    private void dispatchTouchEventToNative(MotionEvent event) {
        int action = event.getActionMasked();
        int actionIndex = event.getActionIndex();
        int pointerCount = event.getPointerCount();

        // Prepare data arrays
        int[] pointerIds = new int[pointerCount];
        float[] x = new float[pointerCount];
        float[] y = new float[pointerCount];
        float[] pressure = new float[pointerCount];

        for (int i = 0; i < pointerCount; i++) {
            pointerIds[i] = event.getPointerId(i);
            x[i] = event.getX(i);
            y[i] = event.getY(i);
            pressure[i] = event.getPressure(i);
        }

        long eventTime = event.getEventTime();

        // Call JNI
        nativeDispatchTouchEvent(
            action,
            actionIndex,
            pointerCount,
            pointerIds,
            x, y, pressure,
            eventTime
        );
    }

    public int mapAndroidKeyCode(int androidKeyCode) {
        switch (androidKeyCode) {
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
                return LF_KEY_ENTER;
            case KeyEvent.KEYCODE_TAB:
                return LF_KEY_TAB;
            case KeyEvent.KEYCODE_DEL:
                return LF_KEY_BACKSPACE;
            case KeyEvent.KEYCODE_ESCAPE:
                return LF_KEY_ESCAPE;
            case KeyEvent.KEYCODE_FORWARD_DEL:
                return LF_KEY_DELETE;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return LF_KEY_LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return LF_KEY_RIGHT;
            case KeyEvent.KEYCODE_DPAD_UP:
                return LF_KEY_UP;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return LF_KEY_DOWN;
            case KeyEvent.KEYCODE_MOVE_HOME:
                return LF_KEY_HOME;
            case KeyEvent.KEYCODE_MOVE_END:
                return LF_KEY_END;
            default:
                return LF_KEY_UNKNOWN;
        }
    }

    public int mapAndroidMetaState(int metaState) {
        int mods = LF_MOD_NONE;
        if ((metaState & KeyEvent.META_SHIFT_ON) != 0) mods |= LF_MOD_SHIFT;
        if ((metaState & KeyEvent.META_CTRL_ON) != 0) mods |= LF_MOD_CTRL;
        if ((metaState & KeyEvent.META_ALT_ON) != 0) mods |= LF_MOD_ALT;
        if ((metaState & KeyEvent.META_META_ON) != 0) mods |= LF_MOD_SUPER;
        return mods;
    }

    public int keyEventTypeDown() {
        return KEY_EVENT_DOWN;
    }

    public int keyEventTypeUp() {
        return KEY_EVENT_UP;
    }
}
