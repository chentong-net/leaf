#define NANOVG_GL3_IMPLEMENTATION

#include "LFAppLaunch.h"
#include "LFEngine.h"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include <mach-o/dyld.h>

namespace {

std::filesystem::path g_assetsRootDir;
constexpr double kWheelStepInLogicalPixels = 32.0;

std::filesystem::path getExecutableDir() {
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);

    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(buffer.c_str()).parent_path();
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
    return path.string();
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    float xscale = 1.0f;
    float yscale = 1.0f;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    LFEngine::getInstance().setWindowSize(
            static_cast<float>(width) / xscale,
            static_cast<float>(height) / yscale,
            xscale
    );
}

double getEngineTime() {
    return LFEngine::getInstance().getElapsedTime();
}

void dispatchMouseToEngine(
        LFTouchEventType type,
        GLFWwindow* window,
        double xpos,
        double ypos) {
    (void)window;

    LFTouchPoint point;
    point.id = 0;
    point.x = static_cast<float>(xpos);
    point.y = static_cast<float>(ypos);
    point.pressure = 1.0f;
    point.timestamp = getEngineTime();

    auto root = LFEngine::getInstance().getRoot();
    if (!root) {
        return;
    }

    const std::vector<LFTouchPoint> touches = {point};
    const std::vector<LFTouchID> changed = {0};
    LFEventDispatcher::getInstance().dispatchTouchEvent(type, touches, changed, root);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)mods;
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);

    const LFTouchEventType type =
            action == GLFW_PRESS ? LFTouchEventType::Down : LFTouchEventType::Up;
    dispatchMouseToEngine(type, window, xpos, ypos);
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        dispatchMouseToEngine(LFTouchEventType::Move, window, xpos, ypos);
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto root = LFEngine::getInstance().getRoot();
    if (!root) return;

    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);

    LFEventDispatcher::getInstance().dispatchScrollEvent(
            static_cast<float>(xpos),
            static_cast<float>(ypos),
            static_cast<float>(xoffset * kWheelStepInLogicalPixels),
            static_cast<float>(-yoffset * kWheelStepInLogicalPixels),
            LFKeyModNone,
            root
    );
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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;

    const LFKeyCode keyCode = toLFKeyCode(key);
    const uint32_t lfMods = toLFKeyMods(mods);

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

void charCallback(GLFWwindow* window, unsigned int codepoint) {
    (void)window;
    LFEventDispatcher::getInstance().dispatchCharInput(codepoint);
}

} // namespace

int main() {
    g_assetsRootDir = getExecutableDir() / "assets";
    if (!std::filesystem::exists(g_assetsRootDir)) {
        const std::string assetsRootUtf8 = pathToUtf8(g_assetsRootDir);
        LF_LOGI("Assets directory does not exist: %s", assetsRootUtf8.c_str());
    }

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(960, 720, "Leaf", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    const int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES;
    NVGcontext* vg = nvgCreateGL3(flags);
    if (!vg) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    LFEngine::getInstance().init(vg);
    LFEngine::getInstance().setTextInputCursorCallback([window](float x, float y, float lineHeight) {
        (void)lineHeight;
        glfwSetInputMethodCursorPos(window, static_cast<double>(x), static_cast<double>(y));
    });

    const std::filesystem::path fontPath =
            resolveAssetPath("fonts/Alibaba-PuHuiTi-Regular.ttf");
    const std::string fontPathUtf8 = pathToUtf8(fontPath);
    if (nvgCreateFont(vg, "sans", fontPathUtf8.c_str()) < 0) {
        LF_LOGI("Failed to load font: %s", fontPathUtf8.c_str());
    }

    LFResourceProvider::getInstance().setAssetLoader(
            [](const std::string& assetPath,
               std::function<void(std::shared_ptr<LFData>)> callback) {
                const std::filesystem::path fsPath = resolveAssetPath(assetPath);
                std::ifstream file(fsPath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    callback(nullptr);
                    return;
                }

                const std::streamsize size = file.tellg();
                if (size <= 0) {
                    callback(nullptr);
                    return;
                }

                file.seekg(0, std::ios::beg);
                auto data = std::make_shared<LFData>();
                data->size = static_cast<size_t>(size);
                data->data = static_cast<unsigned char*>(malloc(data->size));

                if (file.read(reinterpret_cast<char*>(data->data), size)) {
                    callback(data);
                } else {
                    callback(nullptr);
                }
            }
    );

    LFEngine::getInstance().setRoot(createAppRoot());

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    framebufferSizeCallback(window, width, height);

    while (!glfwWindowShouldClose(window)) {
        LFEngine::getInstance().update(0.016f);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        LFEngine::getInstance().render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    nvgDeleteGL3(vg);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
