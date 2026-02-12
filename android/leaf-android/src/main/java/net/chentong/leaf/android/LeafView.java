package net.chentong.leaf.android;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.text.InputType;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

public class LeafView extends GLSurfaceView implements LeafRenderer.TextInputFocusListener {

    private LeafRenderer renderer;
    private final InputMethodManager inputMethodManager;

    public LeafView(Context context) {
        super(context);
        inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        init(context);
    }

    public LeafView(Context context, AttributeSet attrs) {
        super(context, attrs);
        inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        init(context);
    }

    private void init(Context context) {
        // 设置 OpenGL ES 版本为 3.0
        setEGLContextClientVersion(3);

        setFocusable(true);
        setFocusableInTouchMode(true);

        // 显式设置模板缓冲区
        // LFScrollView 依赖 nvgIntersectScissor 来裁剪超出视口的内容
        // NanoVG 内部利用模板缓冲区来标记画的内容
        // setEGLConfigChooser(8, 8, 8, 8, 16, 8);

        // 设置渲染器
        renderer = new LeafRenderer(context.getAssets(), this);
        setRenderer(renderer);

        // 设置渲染模式：CONTINUOUSLY 表示持续刷新（类似游戏/Flutter）
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            requestFocus();
        }

        // Dispatch to renderer (will be queued and processed on GL thread)
        if (renderer != null) {
            renderer.queueTouchEvent(event);
        }
        return true;
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return true;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
        outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_FULLSCREEN;

        return new BaseInputConnection(this, false) {
            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                queueTextAsCharInput(text);
                return true;
            }

            @Override
            public boolean setComposingText(CharSequence text, int newCursorPosition) {
                // 组合态由输入法内部处理，最终 commitText 才写入引擎
                return true;
            }

            @Override
            public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                if (renderer == null) return true;

                int mods = 0;
                for (int i = 0; i < beforeLength; i++) {
                    renderer.queueKeyEvent(renderer.keyEventTypeDown(), renderer.mapAndroidKeyCode(KeyEvent.KEYCODE_DEL), mods, false);
                    renderer.queueKeyEvent(renderer.keyEventTypeUp(), renderer.mapAndroidKeyCode(KeyEvent.KEYCODE_DEL), mods, false);
                }

                for (int i = 0; i < afterLength; i++) {
                    renderer.queueKeyEvent(renderer.keyEventTypeDown(), renderer.mapAndroidKeyCode(KeyEvent.KEYCODE_FORWARD_DEL), mods, false);
                    renderer.queueKeyEvent(renderer.keyEventTypeUp(), renderer.mapAndroidKeyCode(KeyEvent.KEYCODE_FORWARD_DEL), mods, false);
                }
                return true;
            }

            @Override
            public boolean sendKeyEvent(KeyEvent event) {
                return dispatchAndroidKeyEvent(event);
            }
        };
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (dispatchAndroidKeyEvent(event)) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (dispatchAndroidKeyEvent(event)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    private boolean dispatchAndroidKeyEvent(KeyEvent event) {
        if (renderer == null) {
            return false;
        }

        int action = event.getAction();
        if (action != KeyEvent.ACTION_DOWN && action != KeyEvent.ACTION_UP) {
            return false;
        }

        int lfKeyCode = renderer.mapAndroidKeyCode(event.getKeyCode());
        int mods = renderer.mapAndroidMetaState(event.getMetaState());
        int lfType = (action == KeyEvent.ACTION_DOWN) ? renderer.keyEventTypeDown() : renderer.keyEventTypeUp();
        boolean repeat = event.getRepeatCount() > 0;

        boolean handled = false;
        if (lfKeyCode != 0) {
            renderer.queueKeyEvent(lfType, lfKeyCode, mods, repeat);
            handled = true;
        }

        if (action == KeyEvent.ACTION_DOWN && !event.isCtrlPressed() && !event.isAltPressed() && !event.isMetaPressed()) {
            int codepoint = event.getUnicodeChar(event.getMetaState());
            if (lfKeyCode == 0 && codepoint >= 32) {
                renderer.queueCharInput(codepoint);
                handled = true;
            }
        }

        return handled;
    }

    private void queueTextAsCharInput(CharSequence text) {
        if (renderer == null || text == null || text.length() == 0) return;

        int i = 0;
        while (i < text.length()) {
            int codepoint = Character.codePointAt(text, i);
            if (codepoint > 0) {
                renderer.queueCharInput(codepoint);
            }
            i += Character.charCount(codepoint);
        }
    }

    @Override
    public void onTextInputFocusChanged(boolean focused) {
        post(() -> {
            if (inputMethodManager == null) return;

            if (focused) {
                requestFocus();
                inputMethodManager.restartInput(this);
                inputMethodManager.showSoftInput(this, InputMethodManager.SHOW_IMPLICIT);
            } else {
                inputMethodManager.hideSoftInputFromWindow(getWindowToken(), 0);
            }
        });
    }
}
