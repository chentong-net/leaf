LEAF_POSIX_TARGETS="macos ios web android ohos"
LEAF_KNOWN_TARGETS="macos ios web android ohos windows linux"

leaf_target_is_known() {
    case " ${LEAF_KNOWN_TARGETS} " in
        *" $1 "*)
            return 0
            ;;
    esac
    return 1
}

leaf_target_is_posix_supported() {
    case " ${LEAF_POSIX_TARGETS} " in
        *" $1 "*)
            return 0
            ;;
    esac
    return 1
}

resolve_leaf_targets() {
    LEAF_UNIQUE_LIST=

    if [ "$1" = "all" ]; then
        LEAF_RESOLVED_TARGETS="${LEAF_POSIX_TARGETS}"
        return 0
    fi

    for LEAF_RESOLVE_TARGET in $1; do
        if [ "${LEAF_RESOLVE_TARGET}" = "all" ]; then
            leaf_error "target 'all' cannot be combined with other targets"
            return 1
        fi
        if ! leaf_target_is_known "${LEAF_RESOLVE_TARGET}"; then
            printf '[%s] unsupported\n' "${LEAF_RESOLVE_TARGET}"
            return 1
        fi
        leaf_append_unique "${LEAF_RESOLVE_TARGET}"
    done

    if [ -z "${LEAF_UNIQUE_LIST}" ]; then
        leaf_error "no targets were selected"
        return 1
    fi

    LEAF_RESOLVED_TARGETS="${LEAF_UNIQUE_LIST}"
}

leaf_host_arch() {
    case "$(uname -m)" in
        arm64|aarch64)
            printf 'arm64\n'
            ;;
        x86_64|amd64)
            printf 'x64\n'
            ;;
        *)
            uname -m
            ;;
    esac
}

leaf_android_sdk_path() {
    LEAF_ANDROID_SDK_RESOLVED=
    if [ -n "${LEAF_ANDROID_SDK:-}" ] && [ -d "${LEAF_ANDROID_SDK}" ]; then
        LEAF_ANDROID_SDK_RESOLVED=${LEAF_ANDROID_SDK}
    elif [ -n "${ANDROID_SDK_ROOT:-}" ] && [ -d "${ANDROID_SDK_ROOT}" ]; then
        LEAF_ANDROID_SDK_RESOLVED=${ANDROID_SDK_ROOT}
    elif [ -n "${ANDROID_HOME:-}" ] && [ -d "${ANDROID_HOME}" ]; then
        LEAF_ANDROID_SDK_RESOLVED=${ANDROID_HOME}
    elif [ -f "${LEAF_ROOT}/android/local.properties" ]; then
        LEAF_ANDROID_SDK_RESOLVED=$(sed -n 's/^sdk\.dir=//p' \
                "${LEAF_ROOT}/android/local.properties" | head -n 1)
    fi
    printf '%s\n' "${LEAF_ANDROID_SDK_RESOLVED}"
}

leaf_android_java_path() {
    if [ -n "${LEAF_JAVA_HOME:-}" ] &&
       [ -x "${LEAF_JAVA_HOME}/bin/java" ] &&
       "${LEAF_JAVA_HOME}/bin/java" -version >/dev/null 2>&1; then
        printf '%s\n' "${LEAF_JAVA_HOME}/bin/java"
        return 0
    fi
    if [ -n "${JAVA_HOME:-}" ] &&
       [ -x "${JAVA_HOME}/bin/java" ] &&
       "${JAVA_HOME}/bin/java" -version >/dev/null 2>&1; then
        printf '%s\n' "${JAVA_HOME}/bin/java"
        return 0
    fi
    if [ "$(uname -s)" = "Darwin" ] &&
       [ -x "/Applications/Android Studio.app/Contents/jbr/Contents/Home/bin/java" ]; then
        printf '%s\n' "/Applications/Android Studio.app/Contents/jbr/Contents/Home/bin/java"
        return 0
    fi
    if leaf_has_command java && java -version >/dev/null 2>&1; then
        command -v java
        return 0
    fi
    return 1
}

leaf_android_ndk_path() {
    if [ -n "${LEAF_ANDROID_NDK:-}" ] && [ -d "${LEAF_ANDROID_NDK}" ]; then
        printf '%s\n' "${LEAF_ANDROID_NDK}"
        return 0
    fi
    LEAF_ANDROID_SDK_CHECK=$(leaf_android_sdk_path)
    if [ -n "${LEAF_ANDROID_SDK_CHECK}" ] &&
       [ -d "${LEAF_ANDROID_SDK_CHECK}/ndk/28.1.13356709" ]; then
        printf '%s\n' "${LEAF_ANDROID_SDK_CHECK}/ndk/28.1.13356709"
        return 0
    fi
    return 1
}

leaf_emscripten_root() {
    if [ -n "${LEAF_EMSDK:-}" ]; then
        printf '%s\n' "${LEAF_EMSDK}"
    elif [ -d "${LEAF_ROOT}/emsdk" ]; then
        printf '%s\n' "${LEAF_ROOT}/emsdk"
    fi
}

leaf_target_doctor() {
    TARGET_STATUS=unsupported
    TARGET_REASON=

    if ! leaf_target_is_known "$1"; then
        TARGET_REASON="unknown target"
        return 1
    fi

    if ! leaf_target_is_posix_supported "$1"; then
        TARGET_REASON="not included in the POSIX Shell CLI"
        return 1
    fi

    case "$1" in
        macos)
            if [ "$(uname -s)" != "Darwin" ]; then
                TARGET_REASON="requires macOS"
                return 1
            fi
            leaf_has_command cmake || {
                TARGET_REASON="cmake not found"
                return 1
            }
            xcrun --find clang >/dev/null 2>&1 || {
                TARGET_REASON="Apple clang not found"
                return 1
            }
            TARGET_REASON="cmake and Apple clang available"
            ;;
        ios)
            if [ "$(uname -s)" != "Darwin" ]; then
                TARGET_REASON="requires macOS"
                return 1
            fi
            for LEAF_REQUIRED in cmake xcodebuild xcrun pod libtool; do
                leaf_has_command "${LEAF_REQUIRED}" || {
                    TARGET_REASON="${LEAF_REQUIRED} not found"
                    return 1
                }
            done
            TARGET_REASON="Xcode and CocoaPods available"
            ;;
        web)
            case "$(uname -s)" in
                Darwin|Linux)
                    ;;
                *)
                    TARGET_REASON="requires macOS or Linux"
                    return 1
                    ;;
            esac
            LEAF_EMSDK_CHECK=$(leaf_emscripten_root)
            if [ -z "${LEAF_EMSDK_CHECK}" ] ||
               [ ! -f "${LEAF_EMSDK_CHECK}/emsdk_env.sh" ] ||
               [ ! -f "${LEAF_EMSDK_CHECK}/upstream/emscripten/emcmake" ]; then
                TARGET_REASON="Emscripten SDK not found"
                return 1
            fi
            TARGET_REASON="Emscripten available at ${LEAF_EMSDK_CHECK}"
            ;;
        android)
            leaf_has_command cmake || {
                TARGET_REASON="cmake not found"
                return 1
            }
            LEAF_ANDROID_SDK_CHECK=$(leaf_android_sdk_path)
            if [ -z "${LEAF_ANDROID_SDK_CHECK}" ]; then
                TARGET_REASON="Android SDK not found"
                return 1
            fi
            if [ ! -d "${LEAF_ANDROID_SDK_CHECK}/platforms/android-34" ]; then
                TARGET_REASON="Android platform 34 not found"
                return 1
            fi
            if [ ! -d "${LEAF_ANDROID_SDK_CHECK}/build-tools/34.0.0" ]; then
                TARGET_REASON="Android build-tools 34.0.0 not found"
                return 1
            fi
            if [ ! -x "${LEAF_ANDROID_SDK_CHECK}/cmake/3.22.1/bin/cmake" ]; then
                TARGET_REASON="Android SDK CMake 3.22.1 not found"
                return 1
            fi
            leaf_android_java_path >/dev/null || {
                TARGET_REASON="Java runtime not found"
                return 1
            }
            leaf_android_ndk_path >/dev/null || {
                TARGET_REASON="Android NDK 28.1.13356709 not found"
                return 1
            }
            [ -x "${LEAF_ROOT}/android/gradlew" ] || {
                TARGET_REASON="Gradle wrapper not executable"
                return 1
            }
            TARGET_REASON="Android SDK, JDK, NDK, and Gradle available"
            ;;
        ohos)
            if [ -n "${LEAF_HVIGOR:-}" ] &&
               { [ -x "${LEAF_HVIGOR}" ] || leaf_has_command "${LEAF_HVIGOR}"; }; then
                TARGET_REASON="Hvigor available"
            elif leaf_has_command hvigorw; then
                TARGET_REASON="Hvigor available"
            else
                TARGET_REASON="Hvigor not found"
                return 1
            fi
            ;;
    esac

    TARGET_STATUS=ready
    return 0
}
