leaf_adb_path() {
    if [ -n "${LEAF_ADB:-}" ] && [ -x "${LEAF_ADB}" ]; then
        printf '%s\n' "${LEAF_ADB}"
        return 0
    fi
    LEAF_DEVICE_SDK=$(leaf_android_sdk_path)
    if [ -n "${LEAF_DEVICE_SDK}" ] &&
       [ -x "${LEAF_DEVICE_SDK}/platform-tools/adb" ]; then
        printf '%s\n' "${LEAF_DEVICE_SDK}/platform-tools/adb"
        return 0
    fi
    if leaf_has_command adb; then
        command -v adb
        return 0
    fi
    return 1
}

leaf_hdc_path() {
    if [ -n "${LEAF_HDC:-}" ] && [ -x "${LEAF_HDC}" ]; then
        printf '%s\n' "${LEAF_HDC}"
        return 0
    fi
    if leaf_has_command hdc; then
        command -v hdc
        return 0
    fi
    return 1
}

print_leaf_device_header() {
    printf '%-10s %-40s %-12s %-12s %s\n' \
            "TARGET" "ID" "TYPE" "STATE" "NAME"
}

print_android_devices() {
    LEAF_ANDROID_ADB=$(leaf_adb_path) || return 0
    "${LEAF_ANDROID_ADB}" devices -l 2>/dev/null | awk '
        BEGIN {
            OFS = "\t"
        }
        NR == 1 {
            next
        }
        NF == 0 {
            next
        }
        {
            serial = $1
            state = $2
            type = (serial ~ /^emulator-/) ? "emulator" : "physical"
            name = ""
            for (ix = 3; ix <= NF; ++ix) {
                if ($ix ~ /^model:/) {
                    name = $ix
                    sub(/^model:/, "", name)
                }
            }
            print "android", serial, type, state, name
        }
    '
}

print_ios_devices() {
    [ "$(uname -s)" = "Darwin" ] || return 0
    leaf_has_command xcrun || return 0

    xcrun xctrace list devices 2>/dev/null | awk '
        BEGIN {
            OFS = "\t"
            section = ""
        }
        /^== Devices Offline ==$/ {
            section = "offline"
            next
        }
        /^== Devices ==$/ {
            section = "devices"
            next
        }
        /^== Simulators ==$/ {
            section = "simulators"
            next
        }
        /^==/ {
            next
        }
        section == "" {
            next
        }
        {
            copy = $0
            if (gsub(/\(/, "(", copy) < 2) {
                next
            }
            id = $NF
            gsub(/[()]/, "", id)
            if (id !~ /^[0-9A-Fa-f-]+$/) {
                next
            }
            name = $0
            sub(/ \([0-9A-Fa-f-]+\)$/, "", name)
            sub(/ \([^()]*\)$/, "", name)
            type = (section == "simulators") ? "simulator" : "physical"
            state = (section == "offline") ? "offline" : "available"
            print "ios", id, type, state, name
        }
    '
}

print_ohos_devices() {
    LEAF_OHOS_HDC=$(leaf_hdc_path) || return 0
    "${LEAF_OHOS_HDC}" list targets -v 2>/dev/null | awk '
        BEGIN {
            OFS = "\t"
        }
        NF == 0 || $0 == "[Empty]" || $0 ~ /^\[Empty\]/ {
            next
        }
        {
            id = $1
            state = (NF > 1) ? $2 : "available"
            print "ohos", id, "device", state, ""
        }
    '
}

run_device() {
    resolve_leaf_targets "${LEAF_TARGET_REQUEST}" || exit 1
    print_leaf_device_header

    for LEAF_DEVICE_TARGET in ${LEAF_RESOLVED_TARGETS}; do
        case "${LEAF_DEVICE_TARGET}" in
            android)
                print_android_devices
                ;;
            ios)
                print_ios_devices
                ;;
            ohos)
                print_ohos_devices
                ;;
            macos)
                if [ "$(uname -s)" = "Darwin" ]; then
                    printf 'macos\tlocal\thost\tavailable\t%s\n' "$(hostname)"
                fi
                ;;
            web)
                if leaf_has_command python3; then
                    printf 'web\tweb-server\tbrowser\tavailable\tDefault HTTP Server\n'
                fi
                ;;
        esac
    done

    return 0
}
