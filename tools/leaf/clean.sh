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

clean_native_target() {
    case "$1" in
        macos)
            leaf_clean_path "${LEAF_ROOT}/macos/build-macos"
            ;;
        web)
            leaf_clean_path "${LEAF_ROOT}/web/build-web"
            ;;
        android)
            leaf_clean_path "${LEAF_ROOT}/android/app/build"
            leaf_clean_path "${LEAF_ROOT}/android/leaf-android/build"
            leaf_clean_path "${LEAF_ROOT}/android/leaf-plugin/build"
            for LEAF_ANDROID_PLUGIN in \
                    audio_player file_picker i18n local_time path_provider
            do
                leaf_clean_path \
                        "${LEAF_ROOT}/plugins/${LEAF_ANDROID_PLUGIN}/android/module/build"
            done
            ;;
        ios)
            leaf_clean_path "${LEAF_ROOT}/ios/build-ios"
            leaf_clean_path "${LEAF_ROOT}/ios/App/build"
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
