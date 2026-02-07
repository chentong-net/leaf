#ifndef LEAF_LFDEF_H
#define LEAF_LFDEF_H

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#include <android/log.h>
#define LF_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Leaf", __VA_ARGS__)
#endif

#ifdef __WEB__
#include <GLES3/gl3.h>
#include <emscripten.h>
#define LF_LOGI(...) emscripten_log(EM_LOG_CONSOLE, "[Leaf]: " __VA_ARGS__)
#endif

#ifdef __DESKTOP__
#include "glad.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#define LF_LOGI(...) printf("[Leaf]: " __VA_ARGS__); printf("\n")
#endif

extern "C" {
#include "quickjs.h"
}
#include "nanovg.h"
#include "nanovg_gl.h"
#include "Yoga.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>
#include <algorithm>
#include <memory>
#include <cmath>
#include <mutex>
#include <cstdio>
#include <chrono>
#include <cstdint>
#include <limits>
#include <initializer_list>
#include <map>
#include <type_traits>
#include <fstream>

#endif // LEAF_LFDEF_H