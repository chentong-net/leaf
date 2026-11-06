leaf_clean_path() {
    case "$1" in
        "${LEAF_ROOT}"/*)
            ;;
        *)
            leaf_die "refusing to remove path outside the project: $1"
            ;;
    esac

    if [ -e "$1" ] || [ -L "$1" ]; then
        rm -rf -- "$1"
        leaf_info "removed $1"
    fi
}

clean_android_native() {
    LEAF_CLEAN_JAVA=$(leaf_android_java_path) ||
            leaf_die "Android clean requires a Java runtime"
    LEAF_CLEAN_JAVA_HOME=$(dirname "$(dirname "${LEAF_CLEAN_JAVA}")")
    LEAF_CLEAN_ANDROID_SDK=$(leaf_android_sdk_path)
    [ -n "${LEAF_CLEAN_ANDROID_SDK}" ] ||
            leaf_die "Android clean requires an Android SDK"

    JAVA_HOME="${LEAF_CLEAN_JAVA_HOME}" \
    ANDROID_HOME="${LEAF_CLEAN_ANDROID_SDK}" \
    ANDROID_SDK_ROOT="${LEAF_CLEAN_ANDROID_SDK}" \
            "${LEAF_ROOT}/android/gradlew" -p "${LEAF_ROOT}/android" clean
}

clean_ios_native() {
    [ "$(uname -s)" = "Darwin" ] ||
            leaf_die "iOS clean requires macOS"
    leaf_has_command xcodebuild ||
            leaf_die "iOS clean requires xcodebuild"

    for LEAF_CLEAN_IOS_CONFIGURATION in Debug Release; do
        xcodebuild \
                -workspace "${LEAF_ROOT}/ios/App/App.xcworkspace" \
                -scheme App \
                -configuration "${LEAF_CLEAN_IOS_CONFIGURATION}" \
                -derivedDataPath "${LEAF_ROOT}/ios/build-ios/DerivedData" \
                clean
    done
}

clean_native_target() {
    case "$1" in
        macos)
            leaf_clean_path "${LEAF_ROOT}/macos/build-macos"
            ;;
        web)
            leaf_clean_path "${LEAF_ROOT}/web/build-web"
            ;;
        android)
            clean_android_native
            ;;
        ios)
            clean_ios_native
            ;;
        ohos)
            leaf_clean_path "${LEAF_ROOT}/ohos/entry/build"
            leaf_clean_path "${LEAF_ROOT}/ohos/leaf_ohos/build"
            leaf_clean_path "${LEAF_ROOT}/ohos/leaf_plugin/build"
            for LEAF_OHOS_PLUGIN in \
                    audio_player file_picker i18n path_provider
            do
                leaf_clean_path \
                        "${LEAF_ROOT}/plugins/${LEAF_OHOS_PLUGIN}/ohos/module/build"
            done
            ;;
    esac
}

run_clean() {
    resolve_leaf_targets "${LEAF_TARGET_REQUEST}" || exit 1

    for LEAF_CLEAN_TARGET in ${LEAF_RESOLVED_TARGETS}; do
        if ! leaf_target_is_posix_supported "${LEAF_CLEAN_TARGET}"; then
            printf '[%s] unsupported\n' "${LEAF_CLEAN_TARGET}"
            continue
        fi

        leaf_clean_path "${LEAF_ROOT}/build/${LEAF_CLEAN_TARGET}"

        if [ "${LEAF_CLEAN_ALL}" -eq 1 ]; then
            clean_native_target "${LEAF_CLEAN_TARGET}"
        fi
    done
}
