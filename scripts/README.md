# Leaf CLI

`leaf` is the repository-level POSIX Shell command line for environment checks,
native builds, artifact staging, device discovery, and launching the application.

## Commands

```text
leaf doctor [--target <target>...] [--strict]
leaf device [--target <target>...]
leaf build [--target <target>...] [--mode debug|release] [--strict]
leaf run [--target <target>...] [--mode debug|release] [--device <id>] [--strict]
leaf clean [--target <target>...] [--all]
```

Targets supported by the Shell CLI:

```text
macos ios web android ohos
```

The POSIX Shell CLI does not include Windows. Window support will use a separate
`cmd` entry point.

When `--target` is omitted, the command uses `--target all`. Target names after
`--target` are separated by spaces:

```sh
leaf build --target macos web
```

Build and run default to Debug. `doctor`, `device`, and `clean` do not accept a
mode.

## Application Metadata

Application metadata is defined directly in the repository-root `CMakeLists.txt`:

```cmake
set(LEAF_APPLICATION_NAME "leaf-app" CACHE STRING ...)
set(LEAF_APPLICATION_DISPLAY_NAME "Leaf App" CACHE STRING ...)
set(LEAF_APPLICATION_ID "net.chentong.leaf.app" CACHE STRING ...)
```

Before building, `leaf` runs CMake in metadata-only mode. CMake validates the
values and generates:

```text
build/.metadata/metadata.env
```

The Shell script sources that generated file. It does not parse the CMake source
with `sed`. Platform-specific files such as Gradle resources, plist files, and
HarmonyOS resources are updated from the generated metadata.

## Environment

`leaf` loads only the repository-root `.env` file. Copy `.env.example` and use
`export` for values that must reach CMake, Gradle, Xcode, Emscripten, and device
tools:

```sh
cp .env.example .env
```

Values in `.env` intentionally override inherited environment variables.

## Build Outputs

Every successful native build is copied from its native build directory into:

```text
build/<target>/<mode>/
```

Each final directory contains the standard artifact and a `manifest.json`.
Examples:

```text
build/android/debug/leaf-app-android-debug.apk
build/android/release/leaf-app-android-release-unsigned.apk
build/ios/debug/leaf-app-ios-debug-simulator.app
build/ios/debug/leaf-app-ios-debug.ipa
build/ios/debug/leaf-app-ios-debug-dSYMs.zip
build/macos/debug/leaf-app-macos-debug-arm64
build/web/debug/leaf-app-web-debug.html
build/ohos/debug/leaf-app-ohos-debug.hap
```

Native build directories remain available for incremental builds:

```text
android/*/build
ios/build-ios/ios-native
ios/build-ios
macos/build-macos
web/build-web
ohos/*/build
```

## Doctor And Device

`doctor` checks the environment required by each selected target. Unsupported
targets are reported in the output. `--strict` makes any unsupported target
return a non-zero status.

`device` only discovers runnable destinations. It does not build or install the
application. If no device is available, it prints the table header and exits
successfully.

## Build And Run

`run` always invokes `build` before installing or launching:

```text
doctor environment for build
  -> build
  -> verify final artifact
  -> discover or select device
  -> install or launch
```

iOS Debug attempts a signed device IPA. If signing is unavailable, it builds a
Simulator `.app`. iOS Release requires signing and does not fall back. `--strict`
also disables the Debug fallback. Device builds export the IPA and a zip of the
archive dSYMs into the final artifact directory.

Web output is served over HTTP from the final artifact directory. Android
Release is unsigned in the first CLI version and therefore cannot be launched
with `run`.

## Clean

```text
leaf clean
```

removes all final output directories under `build/` while preserving native
build directories.

```text
leaf clean --target android --all
```

invokes Gradle clean.

```text
leaf clean --target ios --all
```

invokes `xcodebuild clean` for Debug and Release while preserving
`ios/build-ios`.

For macOS and Web, `clean --all` removes their project-local `build-macos` and
`build-web` directories.

Plain `clean` does not require a working build environment. Native cleaning for
Android and iOS requires the corresponding toolchain.
