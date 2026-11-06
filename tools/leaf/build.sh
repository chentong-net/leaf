load_leaf_application_metadata() {
    LEAF_METADATA_DIR="${LEAF_ROOT}/build/.metadata"
    LEAF_METADATA_FILE="${LEAF_METADATA_DIR}/metadata.env"
    mkdir -p "${LEAF_METADATA_DIR}"

    leaf_info "generating ${LEAF_METADATA_FILE}"
    cmake --log-level=ERROR \
            -S "${LEAF_ROOT}" \
            -B "${LEAF_METADATA_DIR}" \
            -DLEAF_METADATA_ONLY=ON

    [ -f "${LEAF_METADATA_FILE}" ] || leaf_die \
            "application metadata was not generated: ${LEAF_METADATA_FILE}"
    . "${LEAF_METADATA_FILE}"

    [ -n "${LEAF_APPLICATION_NAME:-}" ] || leaf_die \
            "LEAF_APPLICATION_NAME is missing from ${LEAF_METADATA_FILE}"
    [ -n "${LEAF_APPLICATION_DISPLAY_NAME:-}" ] || leaf_die \
            "LEAF_APPLICATION_DISPLAY_NAME is missing from ${LEAF_METADATA_FILE}"
    [ -n "${LEAF_APPLICATION_ID:-}" ] || leaf_die \
            "LEAF_APPLICATION_ID is missing from ${LEAF_METADATA_FILE}"
    [ -n "${LEAF_APPLICATION_VERSION:-}" ] || leaf_die \
            "LEAF_APPLICATION_VERSION is missing from ${LEAF_METADATA_FILE}"
}

leaf_xml_escape() {
    printf '%s' "$1" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g'
}

leaf_json_escape() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

leaf_sed_escape() {
    printf '%s' "$1" | sed 's/[\\&#]/\\&/g'
}

leaf_replace_if_changed() {
    if cmp -s "$1" "$2"; then
        rm -f "$1"
    else
        mv "$1" "$2"
    fi
}

sync_android_metadata() {
    LEAF_ANDROID_STRINGS="${LEAF_ROOT}/android/app/src/main/res/values/strings.xml"
    LEAF_XML_VALUE=$(leaf_xml_escape "${LEAF_APPLICATION_DISPLAY_NAME}")
    LEAF_SED_VALUE=$(leaf_sed_escape "${LEAF_XML_VALUE}")
    LEAF_TEMP_FILE="${LEAF_ANDROID_STRINGS}.leaf-tmp.$$"
    sed "s#<string name=\"app_name\">.*</string>#<string name=\"app_name\">${LEAF_SED_VALUE}</string>#" \
            "${LEAF_ANDROID_STRINGS}" > "${LEAF_TEMP_FILE}"
    leaf_replace_if_changed "${LEAF_TEMP_FILE}" "${LEAF_ANDROID_STRINGS}"

    LEAF_ANDROID_GRADLE="${LEAF_ROOT}/android/app/build.gradle"
    LEAF_SED_ID=$(leaf_sed_escape "${LEAF_APPLICATION_ID}")
    LEAF_TEMP_FILE="${LEAF_ANDROID_GRADLE}.leaf-tmp.$$"
    sed "s#applicationId .*#applicationId \"${LEAF_SED_ID}\"#" \
            "${LEAF_ANDROID_GRADLE}" > "${LEAF_TEMP_FILE}"
    leaf_replace_if_changed "${LEAF_TEMP_FILE}" "${LEAF_ANDROID_GRADLE}"
}

sync_ohos_metadata() {
    LEAF_OHOS_APP_FILE="${LEAF_ROOT}/ohos/AppScope/app.json5"
    LEAF_SED_ID=$(leaf_sed_escape "${LEAF_APPLICATION_ID}")
    LEAF_TEMP_FILE="${LEAF_OHOS_APP_FILE}.leaf-tmp.$$"
    sed "s#\"bundleName\": \".*\"#\"bundleName\": \"${LEAF_SED_ID}\"#" \
            "${LEAF_OHOS_APP_FILE}" > "${LEAF_TEMP_FILE}"
    leaf_replace_if_changed "${LEAF_TEMP_FILE}" "${LEAF_OHOS_APP_FILE}"

    LEAF_JSON_VALUE=$(leaf_json_escape "${LEAF_APPLICATION_DISPLAY_NAME}")
    for LEAF_OHOS_STRING_FILE in \
            "${LEAF_ROOT}/ohos/AppScope/resources/base/element/string.json" \
            "${LEAF_ROOT}/ohos/entry/src/main/resources/base/element/string.json"
    do
        case "${LEAF_OHOS_STRING_FILE}" in
            */AppScope/*)
                LEAF_OHOS_RESOURCE_NAME=app_name
                ;;
            *)
                LEAF_OHOS_RESOURCE_NAME=EntryAbility_label
                ;;
        esac

        LEAF_TEMP_FILE="${LEAF_OHOS_STRING_FILE}.leaf-tmp.$$"
        if ! LEAF_JSON_VALUE="${LEAF_JSON_VALUE}" \
                LEAF_JSON_KEY="${LEAF_OHOS_RESOURCE_NAME}" awk '
            $0 ~ "\"name\"[[:space:]]*:[[:space:]]*\"" ENVIRON["LEAF_JSON_KEY"] "\"" {
                pending = 1
                found = 1
                print
                next
            }
            pending && $0 ~ /"value"[[:space:]]*:/ {
                print "      \"value\": \"" ENVIRON["LEAF_JSON_VALUE"] "\""
                pending = 0
                next
            }
            {
                print
            }
            END {
                if (!found || pending) {
                    exit 1
                }
            }
        ' "${LEAF_OHOS_STRING_FILE}" > "${LEAF_TEMP_FILE}"; then
            rm -f "${LEAF_TEMP_FILE}"
            leaf_die "failed to update HarmonyOS display name: ${LEAF_OHOS_STRING_FILE}"
        fi
        leaf_replace_if_changed "${LEAF_TEMP_FILE}" "${LEAF_OHOS_STRING_FILE}"
    done
}

sync_ios_metadata() {
    LEAF_IOS_PLIST="${LEAF_ROOT}/ios/App/App/Info.plist"
    LEAF_XML_VALUE=$(leaf_xml_escape "${LEAF_APPLICATION_DISPLAY_NAME}")
    LEAF_TEMP_FILE="${LEAF_IOS_PLIST}.leaf-tmp.$$"
    if ! LEAF_XML_VALUE="${LEAF_XML_VALUE}" awk '
        /<key>CFBundleDisplayName<\/key>/ {
            found = 1
            print
            next
        }
        found {
            print "\t<string>" ENVIRON["LEAF_XML_VALUE"] "</string>"
            found = 0
            replaced = 1
            next
        }
        {
            print
        }
        END {
            if (!replaced) {
                exit 1
            }
        }
    ' "${LEAF_IOS_PLIST}" > "${LEAF_TEMP_FILE}"; then
        rm -f "${LEAF_TEMP_FILE}"
        leaf_die "failed to update CFBundleDisplayName in ${LEAF_IOS_PLIST}"
    fi
    leaf_replace_if_changed "${LEAF_TEMP_FILE}" "${LEAF_IOS_PLIST}"
}

sync_web_title() {
    LEAF_HTML_FILE=$1
    LEAF_XML_VALUE=$(leaf_xml_escape "${LEAF_APPLICATION_DISPLAY_NAME}")
    LEAF_SED_VALUE=$(leaf_sed_escape "${LEAF_XML_VALUE}")
    LEAF_TEMP_FILE="${LEAF_HTML_FILE}.leaf-tmp.$$"
    sed "s#<title>.*</title>#<title>${LEAF_SED_VALUE}</title>#" \
            "${LEAF_HTML_FILE}" > "${LEAF_TEMP_FILE}"
    leaf_replace_if_changed "${LEAF_TEMP_FILE}" "${LEAF_HTML_FILE}"
}

leaf_begin_staging() {
    LEAF_FINAL_DIR="${LEAF_ROOT}/build/$1/$2"
    LEAF_STAGING_DIR="${LEAF_ROOT}/build/$1/.$2.tmp.$$"
    rm -rf -- "${LEAF_STAGING_DIR}"
    mkdir -p "${LEAF_STAGING_DIR}"
}

leaf_promote_staging() {
    [ -n "$(find "${LEAF_STAGING_DIR}" -mindepth 1 -print -quit 2>/dev/null)" ] ||
            leaf_die "target produced no final artifacts"
    mkdir -p "$(dirname "${LEAF_FINAL_DIR}")"
    rm -rf -- "${LEAF_FINAL_DIR}"
    mv "${LEAF_STAGING_DIR}" "${LEAF_FINAL_DIR}"
    leaf_info "artifacts: ${LEAF_FINAL_DIR}"
}

write_leaf_manifest() {
    LEAF_MANIFEST_TARGET=$1
    LEAF_MANIFEST_VARIANT=${2:-}
    LEAF_MANIFEST_VERSION=${LEAF_APPLICATION_VERSION}
    LEAF_MANIFEST_COMMIT=$(git -C "${LEAF_ROOT}" rev-parse HEAD)
    if [ -n "$(git -C "${LEAF_ROOT}" status --porcelain --untracked-files=no)" ]; then
        LEAF_MANIFEST_DIRTY=true
    else
        LEAF_MANIFEST_DIRTY=false
    fi
    LEAF_MANIFEST_NAME=$(leaf_json_escape "${LEAF_APPLICATION_NAME}")
    LEAF_MANIFEST_DISPLAY_NAME=$(
        leaf_json_escape "${LEAF_APPLICATION_DISPLAY_NAME}"
    )
    LEAF_MANIFEST_ID=$(leaf_json_escape "${LEAF_APPLICATION_ID}")
    LEAF_MANIFEST_VARIANT_JSON=$(
        leaf_json_escape "${LEAF_MANIFEST_VARIANT}"
    )

    cat > "${LEAF_STAGING_DIR}/manifest.json" <<EOF
{
  "application": {
    "name": "${LEAF_MANIFEST_NAME}",
    "displayName": "${LEAF_MANIFEST_DISPLAY_NAME}",
    "id": "${LEAF_MANIFEST_ID}",
    "version": "${LEAF_MANIFEST_VERSION}"
  },
  "target": "${LEAF_MANIFEST_TARGET}",
  "mode": "${LEAF_MODE}",
  "variant": "${LEAF_MANIFEST_VARIANT_JSON}",
  "git": {
    "commit": "${LEAF_MANIFEST_COMMIT}",
    "dirty": ${LEAF_MANIFEST_DIRTY}
  },
  "generatedAt": "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
}
EOF
}

leaf_cmake_mode() {
    case "$1" in
        debug)
            printf 'Debug\n'
            ;;
        release)
            printf 'Release\n'
            ;;
    esac
}

build_macos_target() {
    LEAF_CMAKE_MODE=$(leaf_cmake_mode "${LEAF_MODE}")
    LEAF_NATIVE_DIR="${LEAF_ROOT}/macos/build-macos"
    cmake -S "${LEAF_ROOT}/macos" -B "${LEAF_NATIVE_DIR}" \
            "-DCMAKE_BUILD_TYPE=${LEAF_CMAKE_MODE}" \
            "-DLEAF_APPLICATION_NAME=${LEAF_APPLICATION_NAME}" \
            "-DLEAF_APPLICATION_DISPLAY_NAME=${LEAF_APPLICATION_DISPLAY_NAME}" \
            "-DLEAF_APPLICATION_ID=${LEAF_APPLICATION_ID}"
    cmake --build "${LEAF_NATIVE_DIR}" --parallel

    LEAF_SOURCE_BINARY="${LEAF_NATIVE_DIR}/leaf-macos"
    LEAF_SOURCE_ASSETS="${LEAF_NATIVE_DIR}/assets"
    [ -x "${LEAF_SOURCE_BINARY}" ] ||
            leaf_die "macOS executable not found: ${LEAF_SOURCE_BINARY}"
    [ -d "${LEAF_SOURCE_ASSETS}" ] ||
            leaf_die "macOS assets not found: ${LEAF_SOURCE_ASSETS}"

    leaf_begin_staging macos "${LEAF_MODE}"
    LEAF_ARCH=$(leaf_host_arch)
    LEAF_ARTIFACT="${LEAF_FINAL_DIR}/${LEAF_APPLICATION_NAME}-macos-${LEAF_MODE}-${LEAF_ARCH}"
    LEAF_STAGED_ARTIFACT="${LEAF_STAGING_DIR}/$(basename "${LEAF_ARTIFACT}")"
    cp -p "${LEAF_SOURCE_BINARY}" "${LEAF_STAGED_ARTIFACT}"
    cp -R "${LEAF_SOURCE_ASSETS}" "${LEAF_STAGING_DIR}/assets"
    write_leaf_manifest macos "${LEAF_ARCH}"
    leaf_promote_staging
}

build_web_target() {
    LEAF_CMAKE_MODE=$(leaf_cmake_mode "${LEAF_MODE}")
    LEAF_EMSDK_CHECK=$(leaf_emscripten_root)
    [ -n "${LEAF_EMSDK_CHECK}" ] ||
            leaf_die "Emscripten SDK not found"

    EMSDK_QUIET=1 bash -c '
        set -eu
        emsdk_root=$1
        project_root=$2
        cmake_mode=$3
        application_name=$4
        display_name=$5
        application_id=$6
        . "${emsdk_root}/emsdk_env.sh" >/dev/null
        emcmake cmake -S "${project_root}/web" \
            -B "${project_root}/web/build-web" \
            -DCMAKE_BUILD_TYPE="${cmake_mode}" \
            -DLEAF_APPLICATION_NAME="${application_name}" \
            -DLEAF_APPLICATION_DISPLAY_NAME="${display_name}" \
            -DLEAF_APPLICATION_ID="${application_id}"
        cmake --build "${project_root}/web/build-web" --parallel
    ' leaf \
            "${LEAF_EMSDK_CHECK}" \
            "${LEAF_ROOT}" \
            "${LEAF_CMAKE_MODE}" \
            "${LEAF_APPLICATION_NAME}" \
            "${LEAF_APPLICATION_DISPLAY_NAME}" \
            "${LEAF_APPLICATION_ID}"

    LEAF_NATIVE_DIR="${LEAF_ROOT}/web/build-web"
    LEAF_WEB_BASENAME="${LEAF_APPLICATION_NAME}-web-${LEAF_MODE}"
    LEAF_SOURCE_HTML="${LEAF_NATIVE_DIR}/${LEAF_WEB_BASENAME}.html"
    [ -f "${LEAF_SOURCE_HTML}" ] ||
            leaf_die "Web entry not found: ${LEAF_SOURCE_HTML}"

    leaf_begin_staging web "${LEAF_MODE}"
    cp "${LEAF_SOURCE_HTML}" "${LEAF_STAGING_DIR}/${LEAF_WEB_BASENAME}.html"
    for LEAF_WEB_EXTENSION in js wasm data; do
        if [ -f "${LEAF_NATIVE_DIR}/${LEAF_WEB_BASENAME}.${LEAF_WEB_EXTENSION}" ]; then
            cp "${LEAF_NATIVE_DIR}/${LEAF_WEB_BASENAME}.${LEAF_WEB_EXTENSION}" \
                    "${LEAF_STAGING_DIR}/${LEAF_WEB_BASENAME}.${LEAF_WEB_EXTENSION}"
        fi
    done
    if [ -d "${LEAF_NATIVE_DIR}/assets" ]; then
        cp -R "${LEAF_NATIVE_DIR}/assets" "${LEAF_STAGING_DIR}/assets"
    fi
    sync_web_title "${LEAF_STAGING_DIR}/${LEAF_WEB_BASENAME}.html"
    write_leaf_manifest web ""
    leaf_promote_staging
}

build_android_target() {
    sync_android_metadata
    LEAF_JAVA_BINARY=$(leaf_android_java_path) ||
            leaf_die "Java runtime not found"
    LEAF_JAVA_HOME_RESOLVED=$(dirname "$(dirname "${LEAF_JAVA_BINARY}")")
    LEAF_ANDROID_SDK_RESOLVED=$(leaf_android_sdk_path)
    LEAF_ANDROID_NDK_RESOLVED=$(leaf_android_ndk_path) ||
            leaf_die "Android NDK 28.1.13356709 not found"

    case "${LEAF_MODE}" in
        debug)
            LEAF_GRADLE_TASK=:app:assembleDebug
            LEAF_APK_SOURCE="${LEAF_ROOT}/android/app/build/outputs/apk/debug/app-debug.apk"
            LEAF_APK_NAME="${LEAF_APPLICATION_NAME}-android-debug.apk"
            ;;
        release)
            LEAF_GRADLE_TASK=:app:assembleRelease
            LEAF_APK_SOURCE="${LEAF_ROOT}/android/app/build/outputs/apk/release/app-release-unsigned.apk"
            LEAF_APK_NAME="${LEAF_APPLICATION_NAME}-android-release-unsigned.apk"
            ;;
    esac

    JAVA_HOME="${LEAF_JAVA_HOME_RESOLVED}" \
    ANDROID_HOME="${LEAF_ANDROID_SDK_RESOLVED}" \
    ANDROID_SDK_ROOT="${LEAF_ANDROID_SDK_RESOLVED}" \
    ANDROID_NDK_HOME="${LEAF_ANDROID_NDK_RESOLVED}" \
    ANDROID_NDK_ROOT="${LEAF_ANDROID_NDK_RESOLVED}" \
            "${LEAF_ROOT}/android/gradlew" -p "${LEAF_ROOT}/android" "${LEAF_GRADLE_TASK}"

    [ -f "${LEAF_APK_SOURCE}" ] ||
            leaf_die "Android APK not found: ${LEAF_APK_SOURCE}"
    leaf_begin_staging android "${LEAF_MODE}"
    cp "${LEAF_APK_SOURCE}" "${LEAF_STAGING_DIR}/${LEAF_APK_NAME}"
    if [ "${LEAF_MODE}" = "release" ]; then
        write_leaf_manifest android "unsigned"
    else
        write_leaf_manifest android ""
    fi
    leaf_promote_staging
}

leaf_ios_development_team() {
    if [ -n "${LEAF_IOS_DEVELOPMENT_TEAM:-}" ]; then
        printf '%s\n' "${LEAF_IOS_DEVELOPMENT_TEAM}"
        return 0
    fi
    sed -n 's/.*DEVELOPMENT_TEAM = \([A-Za-z0-9]*\);/\1/p' \
            "${LEAF_ROOT}/ios/App/App.xcodeproj/project.pbxproj" |
            head -n 1
}

prepare_ios_project() {
    sync_ios_metadata
    LEAF_IOS_NATIVE_DIR="${LEAF_ROOT}/ios/build-ios"
    mkdir -p "${LEAF_IOS_NATIVE_DIR}"

    CONFIGURATION=$(leaf_cmake_mode "${LEAF_MODE}") \
    LEAF_SKIP_ASSET_SYNC=0 \
    LEAF_IOS_BUILD_DIR="${LEAF_IOS_NATIVE_DIR}/native" \
            "${LEAF_ROOT}/ios/scripts/build_leaf_native_xcframework.sh"

    (
        cd "${LEAF_ROOT}/ios/App"
        pod install
    )
}

build_ios_simulator() {
    LEAF_IOS_CONFIGURATION=$(leaf_cmake_mode "${LEAF_MODE}")
    LEAF_IOS_DERIVED_DATA="${LEAF_IOS_NATIVE_DIR}/DerivedData"
    xcodebuild \
            -workspace "${LEAF_ROOT}/ios/App/App.xcworkspace" \
            -scheme App \
            -configuration "${LEAF_IOS_CONFIGURATION}" \
            -sdk iphonesimulator \
            -destination "generic/platform=iOS Simulator" \
            -derivedDataPath "${LEAF_IOS_DERIVED_DATA}" \
            CODE_SIGNING_ALLOWED=NO \
            build

    LEAF_IOS_APP="${LEAF_IOS_DERIVED_DATA}/Build/Products/${LEAF_IOS_CONFIGURATION}-iphonesimulator/App.app"
    [ -d "${LEAF_IOS_APP}" ] ||
            leaf_die "iOS simulator app not found: ${LEAF_IOS_APP}"

    leaf_begin_staging ios "${LEAF_MODE}"
    cp -R "${LEAF_IOS_APP}" \
            "${LEAF_STAGING_DIR}/${LEAF_APPLICATION_NAME}-ios-${LEAF_MODE}-simulator.app"
    write_leaf_manifest ios "simulator"
    leaf_promote_staging
}

build_ios_device() {
    LEAF_IOS_CONFIGURATION=$(leaf_cmake_mode "${LEAF_MODE}")
    LEAF_IOS_TEAM=$1
    LEAF_IOS_ARCHIVE="${LEAF_IOS_NATIVE_DIR}/LeafApp.xcarchive"
    LEAF_IOS_EXPORT_DIR="${LEAF_IOS_NATIVE_DIR}/export"
    LEAF_IOS_EXPORT_OPTIONS="${LEAF_IOS_NATIVE_DIR}/ExportOptions.plist"
    rm -rf -- "${LEAF_IOS_ARCHIVE}" "${LEAF_IOS_EXPORT_DIR}"
    mkdir -p "${LEAF_IOS_EXPORT_DIR}"

    xcodebuild \
            -workspace "${LEAF_ROOT}/ios/App/App.xcworkspace" \
            -scheme App \
            -configuration "${LEAF_IOS_CONFIGURATION}" \
            -destination "generic/platform=iOS" \
            -archivePath "${LEAF_IOS_ARCHIVE}" \
            DEVELOPMENT_TEAM="${LEAF_IOS_TEAM}" \
            archive

    cat > "${LEAF_IOS_EXPORT_OPTIONS}" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key>
    <string>debugging</string>
    <key>signingStyle</key>
    <string>automatic</string>
    <key>teamID</key>
    <string>${LEAF_IOS_TEAM}</string>
</dict>
</plist>
EOF

    xcodebuild \
            -exportArchive \
            -archivePath "${LEAF_IOS_ARCHIVE}" \
            -exportPath "${LEAF_IOS_EXPORT_DIR}" \
            -exportOptionsPlist "${LEAF_IOS_EXPORT_OPTIONS}"

    LEAF_IOS_IPA=$(find "${LEAF_IOS_EXPORT_DIR}" -maxdepth 1 -type f -name '*.ipa' \
            -print -quit)
    [ -n "${LEAF_IOS_IPA}" ] ||
            leaf_die "iOS IPA not found in ${LEAF_IOS_EXPORT_DIR}"

    leaf_begin_staging ios "${LEAF_MODE}"
    cp "${LEAF_IOS_IPA}" \
            "${LEAF_STAGING_DIR}/${LEAF_APPLICATION_NAME}-ios-${LEAF_MODE}.ipa"
    write_leaf_manifest ios "device"
    leaf_promote_staging
}

build_ios_target() {
    prepare_ios_project
    LEAF_IOS_TEAM=$(leaf_ios_development_team)

    if [ -n "${LEAF_IOS_TEAM}" ] &&
       security find-identity -v -p codesigning 2>/dev/null |
           grep -q '"Apple Development:'; then
        build_ios_device "${LEAF_IOS_TEAM}"
        return
    fi

    if [ "${LEAF_MODE}" = "release" ]; then
        leaf_die "iOS Release requires a development team and signing identity"
    fi

    leaf_warn "iOS signing is unavailable; building the simulator app"
    build_ios_simulator
}

build_ohos_target() {
    sync_ohos_metadata
    if [ -n "${LEAF_HVIGOR:-}" ]; then
        LEAF_HVIGOR_COMMAND=${LEAF_HVIGOR}
    elif leaf_has_command hvigorw; then
        LEAF_HVIGOR_COMMAND=$(command -v hvigorw)
    else
        leaf_die "Hvigor not found"
    fi

    (
        cd "${LEAF_ROOT}/ohos"
        "${LEAF_HVIGOR_COMMAND}" \
                --mode module \
                -p product=default \
                -p module=entry@default \
                -p "buildMode=${LEAF_MODE}" \
                assembleHap \
                --no-daemon
    )

    LEAF_HAP_SOURCE=$(find "${LEAF_ROOT}/ohos/entry/build" -type f -name '*.hap' \
            -print | head -n 1)
    [ -n "${LEAF_HAP_SOURCE}" ] ||
            leaf_die "HarmonyOS HAP not found under ohos/entry/build"

    leaf_begin_staging ohos "${LEAF_MODE}"
    cp "${LEAF_HAP_SOURCE}" \
            "${LEAF_STAGING_DIR}/${LEAF_APPLICATION_NAME}-ohos-${LEAF_MODE}.hap"
    write_leaf_manifest ohos ""
    leaf_promote_staging
}

build_leaf_target() {
    leaf_info "building $1 (${LEAF_MODE})"
    case "$1" in
        macos)
            build_macos_target
            ;;
        web)
            build_web_target
            ;;
        android)
            build_android_target
            ;;
        ios)
            build_ios_target
            ;;
        ohos)
            build_ohos_target
            ;;
        *)
            leaf_die "target '$1' is not buildable"
            ;;
    esac
}

run_build() {
    load_leaf_application_metadata
    resolve_leaf_targets "${LEAF_TARGET_REQUEST}" || exit 1

    LEAF_BUILD_TARGETS=
    LEAF_BUILD_HAS_UNSUPPORTED=0
    for LEAF_CHECK_TARGET in ${LEAF_RESOLVED_TARGETS}; do
        if leaf_target_doctor "${LEAF_CHECK_TARGET}"; then
            if [ -n "${LEAF_BUILD_TARGETS}" ]; then
                LEAF_BUILD_TARGETS="${LEAF_BUILD_TARGETS} ${LEAF_CHECK_TARGET}"
            else
                LEAF_BUILD_TARGETS=${LEAF_CHECK_TARGET}
            fi
        else
            printf '[%s] unsupported: %s\n' \
                    "${LEAF_CHECK_TARGET}" "${TARGET_REASON}"
            LEAF_BUILD_HAS_UNSUPPORTED=1
        fi
    done

    if [ "${LEAF_BUILD_HAS_UNSUPPORTED}" -eq 1 ] &&
       { [ "${LEAF_STRICT}" -eq 1 ] ||
         [ "${LEAF_TARGET_REQUEST}" != "all" ]; }; then
        exit 1
    fi

    [ -n "${LEAF_BUILD_TARGETS}" ] ||
            leaf_die "no supported targets were selected"

    for LEAF_BUILD_TARGET in ${LEAF_BUILD_TARGETS}; do
        build_leaf_target "${LEAF_BUILD_TARGET}"
    done
}
