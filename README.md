## Leaf

**A Cross-Platform App Development Engine in C++**

**Author:** Chen Tong

English | [中文](README-zh.md)

### Design Goals

- Design and implement a cross-platform application development engine that supports building multi-platform applications from a single codebase.
- Abandon native controls and use a custom-drawn UI approach (OpenGL, Metal, etc.) to ensure consistent visual output across all platforms.
- Separate rendering and layout behind independent interfaces to reduce overall coupling and improve extensibility.
- Use a persistent offscreen canvas and drawing-command diffing to enable partial redraws.
- Record drawing commands independently of actual rendering so that switching render backends does not affect partial redraws.
- Provide an extensible plugin mechanism that supports adding platform-specific plugins as needed.
- Adapt the Web platform using WebAssembly.

### Build

The repository provides a unified POSIX Shell entry point:

```bash
./leaf.sh doctor
./leaf.sh build --target macos web
./leaf.sh run --target ios
./leaf.sh device --target android
./leaf.sh clean --target android --all
```

See [scripts/README.md](scripts/README.md) for the full command and
artifact contract.

#### macOS

```bash
cmake -S macos -B build-macos -DCMAKE_BUILD_TYPE=Debug
cmake --build build-macos --parallel
```

#### Windows

Configure `windows/CMakeLists.txt` with the LLVM toolchain and a supported generator such as Ninja:

```powershell
cmake -S windows -B build-windows -G Ninja `
  -DCMAKE_C_COMPILER=clang.exe `
  -DCMAKE_CXX_COMPILER=clang++.exe
cmake --build build-windows --parallel
```

#### Web

The Web target requires Emscripten:

```bash
emcmake cmake -S web -B build_web -DCMAKE_BUILD_TYPE=Release
cmake --build build_web --parallel
```

Fonts under `application/assets/fonts` are preloaded into the virtual `/assets/fonts`
directory. Other application assets are copied to `build_web/assets` and loaded on
demand with `emscripten_fetch`. Serve `build_web` over HTTP; opening the generated
HTML file directly with `file://` will prevent on-demand requests.

#### Android

```bash
cd android
./gradlew :app:assembleDebug
```

#### iOS

Generate the native framework and install pods before opening the workspace:

```bash
./ios/scripts/build_leaf_native_xcframework.sh
cd ios/App
pod install
open App.xcworkspace
```

The iOS deployment target is iOS 15.0 or newer.

### Project Structure

```text
.
├── README.md
├── core # C++ core code
├── ios # iOS
│   ├── App # launch module
│   ├── Leaf_iOS # platform adapter layer
│   ├── Leaf_Plugin # plugin interface definitions
│   ├── cmake # CMake entrypoint
│   └── scripts # build scripts
├── android # Android
│   ├── app # launch module
│   ├── leaf-plugin # plugin interface definitions
│   └── leaf-android # platform adapter layer
├── ohos # HarmonyOS
│   ├── entry # launch module
│   ├── leaf_plugin # plugin interface definitions
│   └── leaf_ohos # platform adapter layer
├── windows # Windows
│   └── main.cpp # platform adapter layer and entrypoint
├── macos # macOS
│   └── main.cpp # platform adapter layer and entrypoint
├── web # Web
│   ├── web_main.cpp # platform adapter layer and entrypoint
│   └── shell.html # HTML template
├── application
│   ├── AppLaunch.cpp # app startup entrypoint
│   └── CMakeLists.txt # third-party libs/plugins used by demos
├── examples # demo app code
│   ├── my_profile # profile page
│   └── reader_app # reader app
└── third_party # third-party dependencies
```

### Plugin Architecture

```text
xxx # plugin name (e.g., file_picker)
├── CMakeLists.txt # CMake file to load this plugin module
├── xxx.h # public interface header (e.g., LFFilePicker.h)
├── xxx.cpp # interface implementation (sends messages to native layer)
├── ios # iOS native implementation (standard CocoaPods module)
├── android # Android native implementation (standard Android module, can be packaged as AAR)
├── ohos # HarmonyOS native implementation (standard HarmonyOS module, can be packaged as HAR)
├── windows # Windows native implementation
├── macos # macOS native implementation
└── web # Web native implementation
```
