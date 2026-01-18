#include <GLES3/gl3.h>
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

#ifdef __ANDROID__
#include <android/log.h>
#define LF_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Leaf", __VA_ARGS__)
#else
#include <cstdio>
#define LF_LOGI(...) printf("[Leaf]: " __VA_ARGS__); printf("\n")
#endif
