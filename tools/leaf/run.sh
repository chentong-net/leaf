LEAF_BACKGROUND_PIDS=
LEAF_RUN_STARTED=0

leaf_add_background_pid() {
    if [ -n "${LEAF_BACKGROUND_PIDS}" ]; then
        LEAF_BACKGROUND_PIDS="${LEAF_BACKGROUND_PIDS} $1"
    else
        LEAF_BACKGROUND_PIDS="$1"
    fi
}

leaf_note_run_unavailable() {
    if [ "${LEAF_TARGET_REQUEST}" != "all" ] ||
       [ "${LEAF_STRICT}" -eq 1 ]; then
        leaf_error "$1"
        LEAF_RUN_UNAVAILABLE_STATUS=1
    else
        leaf_warn "$1"
        LEAF_RUN_UNAVAILABLE_STATUS=2
    fi
}

run_macos_target() {
    LEAF_MACOS_ARCH=$(leaf_host_arch)
    LEAF_MACOS_BINARY="${LEAF_ROOT}/build/macos/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-macos-${LEAF_MODE}-${LEAF_MACOS_ARCH}"
    [ -x "${LEAF_MACOS_BINARY}" ] ||
            leaf_die "macOS artifact not found: ${LEAF_MACOS_BINARY}"

    leaf_info "running ${LEAF_MACOS_BINARY}"
    LEAF_RUN_STARTED=1
    if [ "${LEAF_RUN_SINGLE}" -eq 1 ]; then
        "${LEAF_MACOS_BINARY}"
    else
        "${LEAF_MACOS_BINARY}" &
        leaf_add_background_pid "$!"
    fi
}

run_web_target() {
    LEAF_WEB_DIR="${LEAF_ROOT}/build/web/${LEAF_MODE}"
    LEAF_WEB_ENTRY="${LEAF_WEB_DIR}/${LEAF_APPLICATION_NAME}-web-${LEAF_MODE}.html"
    [ -f "${LEAF_WEB_ENTRY}" ] ||
            leaf_die "Web artifact not found: ${LEAF_WEB_ENTRY}"
    if ! leaf_has_command python3; then
        leaf_note_run_unavailable 'Web run requires python3'
        return "${LEAF_RUN_UNAVAILABLE_STATUS}"
    fi

    LEAF_WEB_PORT=${LEAF_WEB_PORT:-8000}
    leaf_info "serving ${LEAF_WEB_ENTRY} at http://127.0.0.1:${LEAF_WEB_PORT}/$(basename "${LEAF_WEB_ENTRY}")"
    LEAF_RUN_STARTED=1
    if [ "${LEAF_RUN_SINGLE}" -eq 1 ]; then
        python3 -m http.server "${LEAF_WEB_PORT}" \
                --bind 127.0.0.1 \
                --directory "${LEAF_WEB_DIR}"
    else
        python3 -m http.server "${LEAF_WEB_PORT}" \
                --bind 127.0.0.1 \
                --directory "${LEAF_WEB_DIR}" &
        leaf_add_background_pid "$!"
    fi
}

run_android_target() {
    if [ "${LEAF_MODE}" = "release" ]; then
        leaf_note_run_unavailable \
                'Android Release is unsigned and cannot be launched'
        return "${LEAF_RUN_UNAVAILABLE_STATUS}"
    fi

    if ! LEAF_ANDROID_ADB=$(leaf_adb_path); then
        leaf_note_run_unavailable 'Android run requires adb'
        return "${LEAF_RUN_UNAVAILABLE_STATUS}"
    fi
    LEAF_ANDROID_APK="${LEAF_ROOT}/build/android/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-android-${LEAF_MODE}.apk"
    [ -f "${LEAF_ANDROID_APK}" ] ||
            leaf_die "Android artifact not found: ${LEAF_ANDROID_APK}"

    if [ -n "${LEAF_DEVICE_ID}" ]; then
        if ! "${LEAF_ANDROID_ADB}" devices |
            awk -v id="${LEAF_DEVICE_ID}" '$1 == id && $2 == "device" {
                found = 1
            }
            END {
                exit(found ? 0 : 1)
            }'
        then
            leaf_note_run_unavailable \
                    "Android device not available: ${LEAF_DEVICE_ID}"
            return "${LEAF_RUN_UNAVAILABLE_STATUS}"
        fi
        LEAF_ANDROID_DEVICE=${LEAF_DEVICE_ID}
    else
        LEAF_ANDROID_DEVICES=$(
            "${LEAF_ANDROID_ADB}" devices |
                awk '$2 == "device" { print $1 }'
        )
        LEAF_ANDROID_DEVICE_COUNT=$(printf '%s\n' "${LEAF_ANDROID_DEVICES}" |
                sed '/^$/d' | wc -l | tr -d ' ')
        if [ "${LEAF_ANDROID_DEVICE_COUNT}" -eq 0 ]; then
            leaf_note_run_unavailable 'no connected Android device'
            return "${LEAF_RUN_UNAVAILABLE_STATUS}"
        fi
        if [ "${LEAF_ANDROID_DEVICE_COUNT}" -gt 1 ]; then
            leaf_error "multiple Android devices found:"
            printf '%s\n' "${LEAF_ANDROID_DEVICES}" >&2
            leaf_error "select one with --device <id>"
            return 1
        fi
        LEAF_ANDROID_DEVICE=$(printf '%s\n' "${LEAF_ANDROID_DEVICES}" |
                sed -n '1p')
    fi

    leaf_info "installing ${LEAF_ANDROID_APK} on ${LEAF_ANDROID_DEVICE}"
    "${LEAF_ANDROID_ADB}" -s "${LEAF_ANDROID_DEVICE}" install -r \
            "${LEAF_ANDROID_APK}"
    "${LEAF_ANDROID_ADB}" -s "${LEAF_ANDROID_DEVICE}" shell \
            am start -n "${LEAF_APPLICATION_ID}/.MainActivity"
    LEAF_RUN_STARTED=1
}

leaf_ios_simulator_id() {
    if [ -n "${LEAF_DEVICE_ID}" ]; then
        printf '%s\n' "${LEAF_DEVICE_ID}"
        return 0
    fi

    LEAF_IOS_BOOTED=$(
        xcrun simctl list devices booted 2>/dev/null |
            sed -n 's/.*(\([^)]*\)) (Booted).*/\1/p' |
            sed -n '1p'
    )
    if [ -n "${LEAF_IOS_BOOTED}" ]; then
        printf '%s\n' "${LEAF_IOS_BOOTED}"
        return 0
    fi

    LEAF_IOS_AVAILABLE=$(
        xcrun simctl list devices available 2>/dev/null |
            awk '/iPhone/ && /Shutdown/' |
            sed -n 's/.*(\([^)]*\)) (Shutdown).*/\1/p' |
            sed -n '1p'
    )
    [ -n "${LEAF_IOS_AVAILABLE}" ] || return 1
    printf '%s\n' "${LEAF_IOS_AVAILABLE}"
}

run_ios_simulator() {
    LEAF_IOS_APP="${LEAF_ROOT}/build/ios/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-ios-${LEAF_MODE}-simulator.app"
    [ -d "${LEAF_IOS_APP}" ] ||
            leaf_die "iOS simulator artifact not found: ${LEAF_IOS_APP}"
    if ! LEAF_IOS_DEVICE=$(leaf_ios_simulator_id); then
        leaf_note_run_unavailable 'no available iOS simulator'
        return "${LEAF_RUN_UNAVAILABLE_STATUS}"
    fi

    xcrun simctl boot "${LEAF_IOS_DEVICE}" >/dev/null 2>&1 || true
    open -a Simulator >/dev/null 2>&1 || true
    LEAF_IOS_BUNDLE_ID=$(
        /usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' \
                "${LEAF_IOS_APP}/Info.plist"
    )
    leaf_info "installing ${LEAF_IOS_APP} on simulator ${LEAF_IOS_DEVICE}"
    xcrun simctl install "${LEAF_IOS_DEVICE}" "${LEAF_IOS_APP}"
    xcrun simctl launch "${LEAF_IOS_DEVICE}" "${LEAF_IOS_BUNDLE_ID}"
    LEAF_RUN_STARTED=1
}

run_ios_physical() {
    if [ -z "${LEAF_DEVICE_ID}" ]; then
        leaf_note_run_unavailable \
                'installing an iOS IPA requires --device <id>'
        return "${LEAF_RUN_UNAVAILABLE_STATUS}"
    fi
    LEAF_IOS_IPA="${LEAF_ROOT}/build/ios/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-ios-${LEAF_MODE}.ipa"
    [ -f "${LEAF_IOS_IPA}" ] ||
            leaf_die "iOS IPA not found: ${LEAF_IOS_IPA}"
    LEAF_IOS_RUN_DIR="${LEAF_ROOT}/ios/build-ios/run"
    rm -rf -- "${LEAF_IOS_RUN_DIR}"
    mkdir -p "${LEAF_IOS_RUN_DIR}"
    unzip -q "${LEAF_IOS_IPA}" -d "${LEAF_IOS_RUN_DIR}"
    LEAF_IOS_APP=$(find "${LEAF_IOS_RUN_DIR}/Payload" -maxdepth 1 -type d \
            -name '*.app' -print -quit)
    [ -n "${LEAF_IOS_APP}" ] ||
            leaf_die "could not extract Payload/*.app from ${LEAF_IOS_IPA}"
    LEAF_IOS_BUNDLE_ID=$(
        /usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' \
                "${LEAF_IOS_APP}/Info.plist"
    )

    xcrun devicectl device install app \
            --device "${LEAF_DEVICE_ID}" \
            "${LEAF_IOS_APP}"
    xcrun devicectl device process launch \
            --device "${LEAF_DEVICE_ID}" \
            --terminate-existing \
            "${LEAF_IOS_BUNDLE_ID}"
    LEAF_RUN_STARTED=1
}

run_ios_target() {
    LEAF_IOS_SIMULATOR_APP="${LEAF_ROOT}/build/ios/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-ios-${LEAF_MODE}-simulator.app"
    LEAF_IOS_IPA="${LEAF_ROOT}/build/ios/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-ios-${LEAF_MODE}.ipa"
    if [ -d "${LEAF_IOS_SIMULATOR_APP}" ]; then
        run_ios_simulator
    elif [ -f "${LEAF_IOS_IPA}" ]; then
        run_ios_physical
    else
        leaf_die "no iOS runnable artifact found"
    fi
}

run_ohos_target() {
    if ! LEAF_OHOS_HDC=$(leaf_hdc_path); then
        leaf_note_run_unavailable 'HarmonyOS run requires hdc'
        return "${LEAF_RUN_UNAVAILABLE_STATUS}"
    fi
    LEAF_OHOS_HAP="${LEAF_ROOT}/build/ohos/${LEAF_MODE}/${LEAF_APPLICATION_NAME}-ohos-${LEAF_MODE}.hap"
    [ -f "${LEAF_OHOS_HAP}" ] ||
            leaf_die "HarmonyOS artifact not found: ${LEAF_OHOS_HAP}"

    if [ -z "${LEAF_DEVICE_ID}" ]; then
        LEAF_OHOS_DEVICES=$("${LEAF_OHOS_HDC}" list targets 2>/dev/null |
                sed '/^$/d; /^\[Empty\]/d')
        LEAF_OHOS_DEVICE_COUNT=$(printf '%s\n' "${LEAF_OHOS_DEVICES}" |
                sed '/^$/d' | wc -l | tr -d ' ')
        if [ "${LEAF_OHOS_DEVICE_COUNT}" -eq 0 ]; then
            leaf_note_run_unavailable 'no connected HarmonyOS device'
            return "${LEAF_RUN_UNAVAILABLE_STATUS}"
        fi
        if [ "${LEAF_OHOS_DEVICE_COUNT}" -gt 1 ]; then
            leaf_error "multiple HarmonyOS devices found"
            return 1
        fi
        LEAF_DEVICE_ID=$(printf '%s\n' "${LEAF_OHOS_DEVICES}" | sed -n '1p')
    fi

    "${LEAF_OHOS_HDC}" -t "${LEAF_DEVICE_ID}" install "${LEAF_OHOS_HAP}"
    "${LEAF_OHOS_HDC}" -t "${LEAF_DEVICE_ID}" shell \
            aa start -a EntryAbility -b "${LEAF_APPLICATION_ID}"
    LEAF_RUN_STARTED=1
}

run_leaf_target() {
    case "$1" in
        macos)
            run_macos_target
            ;;
        web)
            run_web_target
            ;;
        android)
            run_android_target
            ;;
        ios)
            run_ios_target
            ;;
        ohos)
            run_ohos_target
            ;;
        *)
            return 1
            ;;
    esac
}

leaf_run_cleanup() {
    trap - INT TERM
    for LEAF_RUN_PID in ${LEAF_BACKGROUND_PIDS}; do
        kill "${LEAF_RUN_PID}" >/dev/null 2>&1 || true
    done
    exit 130
}

run_run() {
    run_build
    LEAF_RUN_SINGLE=0
    set -- ${LEAF_BUILD_TARGETS}
    if [ "$#" -eq 1 ]; then
        LEAF_RUN_SINGLE=1
    fi
    trap leaf_run_cleanup INT TERM

    for LEAF_RUN_TARGET in ${LEAF_BUILD_TARGETS}; do
        if run_leaf_target "${LEAF_RUN_TARGET}"; then
            continue
        else
            LEAF_RUN_RESULT=$?
            if [ "${LEAF_RUN_RESULT}" -eq 1 ]; then
                exit 1
            fi
        fi
    done

    if [ -n "${LEAF_BACKGROUND_PIDS}" ]; then
        for LEAF_RUN_PID in ${LEAF_BACKGROUND_PIDS}; do
            wait "${LEAF_RUN_PID}" || true
        done
    fi
    trap - INT TERM
}
