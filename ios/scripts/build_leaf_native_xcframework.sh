#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NATIVE_DIR="${ROOT_DIR}/ios/native"
BUILD_ROOT="${ROOT_DIR}/build/ios-native"
OUTPUT_DIR="${ROOT_DIR}/ios/Leaf_iOS/Frameworks"
XCFRAMEWORK_PATH="${OUTPUT_DIR}/LeafNative.xcframework"
HEADERS_DIR="${BUILD_ROOT}/headers"
CONFIGURATION="${CONFIGURATION:-Release}"
DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-13.0}"

required_tools=(cmake xcodebuild libtool xcrun)
for tool in "${required_tools[@]}"; do
  if ! command -v "${tool}" >/dev/null 2>&1; then
    echo "error: '${tool}' is required" >&2
    exit 1
  fi
done

build_variant() {
  local name="$1"
  local sysroot="$2"
  local archs="$3"
  local build_dir="${BUILD_ROOT}/${name}"
  local cc
  local cxx

  cc="$(xcrun --sdk "${sysroot}" --find clang)"
  cxx="$(xcrun --sdk "${sysroot}" --find clang++)"

  rm -rf "${build_dir}"

  cmake -S "${NATIVE_DIR}" -B "${build_dir}" \
    -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    -DCMAKE_OSX_SYSROOT="${sysroot}" \
    -DCMAKE_OSX_ARCHITECTURES="${archs}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}" \
    -DCMAKE_C_COMPILER="${cc}" \
    -DCMAKE_CXX_COMPILER="${cxx}"

  cmake --build "${build_dir}" --config "${CONFIGURATION}" --target leaf-native-all
}

find_archive() {
  local build_dir="$1"
  local archive_name="$2"
  local matches=()
  local match
  while IFS= read -r match; do
    matches+=("${match}")
  done < <(find "${build_dir}" -type f -name "${archive_name}" | sort)

  if [[ ${#matches[@]} -eq 0 ]]; then
    echo "error: missing archive '${archive_name}' in ${build_dir}" >&2
    exit 1
  fi

  for match in "${matches[@]}"; do
    if [[ "${match}" != *"/Objects-normal/"* && "${match}" != *"/Binary/"* ]]; then
      echo "${match}"
      return
    fi
  done

  echo "${matches[0]}"
}

merge_archives() {
  local build_dir="$1"
  local output="$2"

  local archive_names=(
    "libleaf-core.a"
    "libfile-picker.a"
    "libpath-provider.a"
    "libmy-profile.a"
    "libreader-app.a"
    "libapp-adapter.a"
    "libleaf-native-anchor.a"
  )

  local archives=()
  local archive
  for archive_name in "${archive_names[@]}"; do
    archive="$(find_archive "${build_dir}" "${archive_name}")"
    archives+=("${archive}")
  done

  rm -f "${output}"
  libtool -static "${archives[@]}" -o "${output}"
}

prepare_headers() {
  rm -rf "${HEADERS_DIR}"
  mkdir -p "${HEADERS_DIR}"

  copy_headers_tree() {
    local source_root="$1"
    while IFS= read -r -d '' header_file; do
      local relative_path="${header_file#${source_root}/}"
      local target_dir="${HEADERS_DIR}/$(dirname "${relative_path}")"
      mkdir -p "${target_dir}"
      cp "${header_file}" "${target_dir}/"
    done < <(find "${source_root}" -type f -name '*.h' -print0)
  }

  # Core public headers
  copy_headers_tree "${ROOT_DIR}/core"

  # Third-party headers referenced by core headers
  copy_headers_tree "${ROOT_DIR}/third_party/yoga"
  while IFS= read -r -d '' header_file; do
    cp "${header_file}" "${HEADERS_DIR}/"
  done < <(find "${ROOT_DIR}/third_party/quickjs" -maxdepth 1 -type f -name '*.h' -print0)
  while IFS= read -r -d '' header_file; do
    cp "${header_file}" "${HEADERS_DIR}/"
  done < <(find "${ROOT_DIR}/third_party/nanovg/src" -maxdepth 1 -type f -name '*.h' -print0)

  # Plugin and demo headers (for SDK-side and host-side extensions)
  copy_headers_tree "${ROOT_DIR}/plugins/file_picker"
  copy_headers_tree "${ROOT_DIR}/plugins/path_provider"
  copy_headers_tree "${ROOT_DIR}/examples/my_profile"
  copy_headers_tree "${ROOT_DIR}/examples/reader_app"

  cat > "${HEADERS_DIR}/leaf_native.h" <<'EOF'
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int leaf_native_anchor_symbol(void);
#ifdef __cplusplus
}
#endif
EOF
}

echo "==> Building iOS device static archives"
build_variant "device" "iphoneos" "arm64"

echo "==> Building iOS simulator static archives"
build_variant "simulator" "iphonesimulator" "arm64;x86_64"

DEVICE_LIB="${BUILD_ROOT}/device/libleaf-native.a"
SIM_LIB="${BUILD_ROOT}/simulator/libleaf-native.a"

echo "==> Merging module archives"
merge_archives "${BUILD_ROOT}/device" "${DEVICE_LIB}"
merge_archives "${BUILD_ROOT}/simulator" "${SIM_LIB}"

prepare_headers
mkdir -p "${OUTPUT_DIR}"
rm -rf "${XCFRAMEWORK_PATH}"

echo "==> Creating LeafNative.xcframework"
xcodebuild -create-xcframework \
  -library "${DEVICE_LIB}" -headers "${HEADERS_DIR}" \
  -library "${SIM_LIB}" -headers "${HEADERS_DIR}" \
  -output "${XCFRAMEWORK_PATH}"

echo "==> Done: ${XCFRAMEWORK_PATH}"
