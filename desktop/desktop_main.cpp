//
// Created by Chen Tong on 2026/2/7.
//

#define NANOVG_GL3_IMPLEMENTATION

#include "LFEngine.h"
#include "ReaderApp.h"
#include "ProfilePage.h"

// 窗口大小改变的回调
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    // 获取设备像素比
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    LFEngine::getInstance().setWindowSize((float) width / xscale, (float) height / yscale, xscale);
}

// 获取当前引擎时间（秒）
double get_engine_time() {
    return LFEngine::getInstance().getElapsedTime();
}

// 统一分发给引擎
void dispatch_mouse_to_engine(LFTouchEventType type, GLFWwindow* window, double xpos, double ypos) {
    // 获取设备缩放比
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    LFTouchPoint p;
    p.id = 0; // 鼠标固定 ID 为 0
    // 将物理坐标转换回逻辑坐标
    p.x = (float)xpos;
    p.y = (float)ypos;
    p.pressure = 1.0f;
    p.timestamp = get_engine_time();

    std::vector<LFTouchPoint> touches = { p };
    std::vector<LFTouchID> changed = { 0 };

    auto root = LFEngine::getInstance().getRoot();
    if (root) {
        LFEventDispatcher::getInstance().dispatchTouchEvent(type, touches, changed, root);
    }
}

// 鼠标按键回调
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    LFTouchEventType type = (action == GLFW_PRESS) ? LFTouchEventType::Down : LFTouchEventType::Up;
    dispatch_mouse_to_engine(type, window, xpos, ypos);
}

// 鼠标移动回调
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // 只有在左键按下时，桌面端才通常分发 Move 事件给移动端 UI 引擎
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        dispatch_mouse_to_engine(LFTouchEventType::Move, window, xpos, ypos);
    }
}

uint32_t toLFKeyMods(int glfwMods) {
    uint32_t mods = LFKeyModNone;
    if (glfwMods & GLFW_MOD_SHIFT) mods |= LFKeyModShift;
    if (glfwMods & GLFW_MOD_CONTROL) mods |= LFKeyModCtrl;
    if (glfwMods & GLFW_MOD_ALT) mods |= LFKeyModAlt;
    if (glfwMods & GLFW_MOD_SUPER) mods |= LFKeyModSuper;
    return mods;
}

LFKeyCode toLFKeyCode(int key) {
    switch (key) {
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            return LFKeyCode::Enter;
        case GLFW_KEY_TAB:
            return LFKeyCode::Tab;
        case GLFW_KEY_BACKSPACE:
            return LFKeyCode::Backspace;
        case GLFW_KEY_ESCAPE:
            return LFKeyCode::Escape;
        case GLFW_KEY_DELETE:
            return LFKeyCode::Delete;
        case GLFW_KEY_LEFT:
            return LFKeyCode::Left;
        case GLFW_KEY_RIGHT:
            return LFKeyCode::Right;
        case GLFW_KEY_UP:
            return LFKeyCode::Up;
        case GLFW_KEY_DOWN:
            return LFKeyCode::Down;
        case GLFW_KEY_HOME:
            return LFKeyCode::Home;
        case GLFW_KEY_END:
            return LFKeyCode::End;
        default:
            return LFKeyCode::Unknown;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    LFKeyCode keyCode = toLFKeyCode(key);
    uint32_t lfMods = toLFKeyMods(mods);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        LFEventDispatcher::getInstance().dispatchKeyEvent(
            LFKeyEventType::Down,
            keyCode,
            lfMods,
            action == GLFW_REPEAT
        );
    } else if (action == GLFW_RELEASE) {
        LFEventDispatcher::getInstance().dispatchKeyEvent(
            LFKeyEventType::Up,
            keyCode,
            lfMods,
            false
        );
    }
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
    LFEventDispatcher::getInstance().dispatchCharInput(codepoint);
}

int main() {
    // 初始化 GLFW
    if (!glfwInit()) return -1;

    // 配置 OpenGL 版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWwindow *window = glfwCreateWindow(360, 640, "Leaf", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, char_callback);


    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        return -1;
    }

    // 初始化 NanoVG
    int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext *vg = nvgCreateGL3(flags);
    if (!vg) return -1;

    // 初始化引擎
    LFEngine::getInstance().init(vg);
    LFEngine::getInstance().setTextInputCursorCallback([window](float x, float y, float lineHeight) {
        (void) lineHeight;
        glfwSetTextInputCursorPos(window, (double) x, (double) y);
    });

    // 加载字体
    nvgCreateFont(vg, "sans", "fonts/Alibaba-PuHuiTi-Regular.ttf");

    LFResourceProvider::getInstance().setAssetLoader(
        [](const std::string &path, std::function<void(std::shared_ptr<LFData>)> callback) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);

            if (!file.is_open()) {
                LF_LOGI("Failed to open file: %s", path.c_str());
                callback(nullptr);
                return;
            }

            // 获取文件大小
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            // 分配内存并读取内容
            auto data = std::make_shared<LFData>();
            data->size = static_cast<size_t>(size);
            data->data = (unsigned char *) malloc(data->size);

            if (file.read(reinterpret_cast<char *>(data->data), size)) {
                // 读取成功，执行回调
                callback(data);
            } else {
                LF_LOGI("Failed to read file content: %s", path.c_str());
                free(data->data);
                callback(nullptr);
            }
        }
    );

    // 设置 Root View
    auto readerApp = ReaderApp::create();
    LFEngine::getInstance().setRoot(readerApp->start());

    // 初始化一次窗口尺寸
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    framebuffer_size_callback(window, w, h);

    // 桌面端主循环
    while (!glfwWindowShouldClose(window)) {
        // 计算 DeltaTime (简单处理取 16ms)
        float deltaTime = 0.016f;

        // 更新逻辑
        LFEngine::getInstance().update(deltaTime);

        // 渲染
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        LFEngine::getInstance().render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理
    LF_LOGI("Close");
    nvgDeleteGL3(vg);
    glfwTerminate();
    return 0;
}
