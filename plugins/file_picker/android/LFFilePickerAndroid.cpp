#include "../cpp/LFFilePicker.h"

#include <jni.h>

namespace {

JavaVM* g_javaVM = nullptr;
jclass g_bridgeClass = nullptr;
jmethodID g_openMethod = nullptr;

JNIEnv* getJNIEnv(bool* needDetach) {
    if (!g_javaVM) {
        return nullptr;
    }

    *needDetach = false;
    JNIEnv* env = nullptr;
    int ret = g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (ret == JNI_OK) {
        return env;
    }

    if (ret == JNI_EDETACHED) {
        if (g_javaVM->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            return nullptr;
        }
        *needDetach = true;
        return env;
    }
    return nullptr;
}

std::string toStdString(JNIEnv* env, jstring value) {
    if (!value) {
        return "";
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) {
        return "";
    }
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void) reserved;
    g_javaVM = vm;
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK && env) {
        jclass localClass = env->FindClass("net/chentong/leaf/filepicker/LeafFilePickerBridge");
        if (localClass) {
            g_bridgeClass = static_cast<jclass>(env->NewGlobalRef(localClass));
            env->DeleteLocalRef(localClass);
            if (g_bridgeClass) {
                g_openMethod = env->GetStaticMethodID(g_bridgeClass, "openFilePicker", "(IIZ)V");
            }
        }
    }
    return JNI_VERSION_1_6;
}

bool lfFilePickerRequestFromPlatform(int requestId, const LFFilePickerOptions& options, std::string& error) {
    bool needDetach = false;
    JNIEnv* env = getJNIEnv(&needDetach);
    if (!env) {
        error = "jni_env_unavailable";
        return false;
    }

    jclass bridgeClass = g_bridgeClass;
    jmethodID openMethod = g_openMethod;

    if (!bridgeClass || !openMethod) {
        bridgeClass = env->FindClass("net/chentong/leaf/filepicker/LeafFilePickerBridge");
        if (bridgeClass) {
            openMethod = env->GetStaticMethodID(bridgeClass, "openFilePicker", "(IIZ)V");
        }
    }

    if (!bridgeClass) {
        error = "bridge_class_not_found";
        if (needDetach) {
            g_javaVM->DetachCurrentThread();
        }
        return false;
    }

    if (!openMethod) {
        error = "open_method_not_found";
        if (needDetach) {
            g_javaVM->DetachCurrentThread();
        }
        return false;
    }

    env->CallStaticVoidMethod(
            bridgeClass,
            openMethod,
            static_cast<jint>(requestId),
            static_cast<jint>(options.mediaType),
            options.copyToSandbox ? JNI_TRUE : JNI_FALSE);
    if (bridgeClass != g_bridgeClass) {
        env->DeleteLocalRef(bridgeClass);
    }

    if (needDetach) {
        g_javaVM->DetachCurrentThread();
    }
    return true;
}

extern "C" JNIEXPORT void JNICALL
Java_net_chentong_leaf_filepicker_LeafFilePickerBridge_nativeOnFilePickerResult(
        JNIEnv* env,
        jclass clazz,
        jint requestId,
        jboolean success,
        jobjectArray paths,
        jstring error
) {
    (void) clazz;

    std::string firstPath;
    if (paths) {
        jsize count = env->GetArrayLength(paths);
        if (count > 0) {
            auto first = static_cast<jstring>(env->GetObjectArrayElement(paths, 0));
            firstPath = toStdString(env, first);
            env->DeleteLocalRef(first);
        }
    }

    std::string errorText = toStdString(env, error);
    lfFilePickerOnPlatformResult(
            static_cast<int>(requestId),
            success == JNI_TRUE ? 1 : 0,
            firstPath.c_str(),
            errorText.c_str());
}
