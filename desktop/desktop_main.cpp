//
// Created by Chen Tong on 2026/2/7.
//

#define NANOVG_GL3_IMPLEMENTATION

#include <array>
#include <filesystem>
#include "LFEngine.h"
#include "LFAppLaunch.h"
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <unistd.h>
#endif

std::filesystem::path g_assetsRootDir;

std::filesystem::path getExecutableDir() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) {
        return std::filesystem::current_path();
    }
    buffer.resize(static_cast<size_t>(len));
    return std::filesystem::path(buffer).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer.c_str()).parent_path();
#else
    std::array<char, 4096> buffer{};
    const ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (len <= 0) {
        return std::filesystem::current_path();
    }
    buffer[static_cast<size_t>(len)] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();
#endif
}

std::filesystem::path resolveAssetPath(const std::string& assetPath) {
    const std::filesystem::path relative = std::filesystem::u8path(assetPath);
    if (relative.is_absolute()) {
        return relative;
    }

    if (!g_assetsRootDir.empty()) {
        const std::filesystem::path resolved = g_assetsRootDir / relative;
        if (std::filesystem::exists(resolved)) {
            return resolved;
        }
    }

    return relative;
}

std::string pathToUtf8(const std::filesystem::path& path) {
#if defined(_WIN32)
    return path.u8string();
#else
    return path.string();
#endif
}

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
#ifdef __APPLE__
    // macOS
    // GLFW 返回的坐标通常直接匹配逻辑坐标系，无需手动除以缩放比
    p.x = (float)xpos;
    p.y = (float)ypos;
#else
    p.x = (xscale > 0) ? (float)xpos / xscale : (float)xpos;
    p.y = (yscale > 0) ? (float)ypos / yscale : (float)ypos;
#endif
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
    g_assetsRootDir = getExecutableDir() / "assets";
    if (!std::filesystem::exists(g_assetsRootDir)) {
        const std::string assetsRootUtf8 = pathToUtf8(g_assetsRootDir);
        LF_LOGI("Assets directory does not exist: %s", assetsRootUtf8.c_str());
    }
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
    GLFWwindow *window = glfwCreateWindow(960, 720, "Leaf", nullptr, nullptr);
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
        glfwSetInputMethodCursorPos(window, (double) x, (double) y);
    });

    // 加载字体
    const std::filesystem::path fontPath = resolveAssetPath("fonts/Alibaba-PuHuiTi-Regular.ttf");
    const std::string fontPathUtf8 = pathToUtf8(fontPath);
    if (nvgCreateFont(vg, "sans", fontPathUtf8.c_str()) < 0) {
        LF_LOGI("Failed to load font: %s", fontPathUtf8.c_str());
    }

    LFResourceProvider::getInstance().setAssetLoader(
        [](const std::string &assetPath, std::function<void(std::shared_ptr<LFData>)> callback) {
            std::filesystem::path fsPath = resolveAssetPath(assetPath);
            std::ifstream file(fsPath, std::ios::binary | std::ios::ate);

            if (!file.is_open()) {
                std::filesystem::path absPath = std::filesystem::absolute(fsPath);
                const std::string resolvedPathUtf8 = pathToUtf8(absPath);
                LF_LOGI("Failed to open file: %s (Resolved: %s)", assetPath.c_str(), resolvedPathUtf8.c_str());
                callback(nullptr);
                return;
            }

            // 获取文件大小
            std::streamsize size = file.tellg();
            if (size <= 0) {
                LF_LOGI("File is empty: %s", assetPath.c_str());
                callback(nullptr);
                return;
            }

            file.seekg(0, std::ios::beg);

            // 分配内存并读取内容
            auto data = std::make_shared<LFData>();
            data->size = static_cast<size_t>(size);
            data->data = (unsigned char *) malloc(data->size);

            if (file.read(reinterpret_cast<char *>(data->data), size)) {
                // 读取成功，执行回调
                callback(data);
            } else {
                LF_LOGI("Failed to read file content: %s", assetPath.c_str());
                free(data->data);
                callback(nullptr);
            }
        }
    );

    // 设置 Root View
    LFEngine::getInstance().setRoot(createAppRoot());

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
