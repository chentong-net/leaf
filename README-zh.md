## Leaf

**C++跨平台应用开发引擎**

**作者:** Chen Tong

[English](README.md) | 中文

### 设计目标

- 设计实现一个跨平台应用开发引擎，实现全平台统一的UI绘制
- 摒弃原生控件，采用自绘UI策略，保证全平台绘制效果的一致
- 组件通过 `LFPaint` 和 `LFRenderer` 使用后端无关的绘制接口；当前默认后端为 NanoVG，在多数平台通过 OpenGL/OpenGL ES 绘制，在 iOS 使用 MetalNanoVG
- 布局通过后端无关的 `LFLayout` 接口计算，当前默认后端为 `LFYogaLayout`，与图形绘制严格解耦

### 构建

#### macOS

```bash
cmake -S macos -B build-macos -DCMAKE_BUILD_TYPE=Debug
cmake --build build-macos --parallel
ctest --test-dir build-macos --output-on-failure
```

#### Windows

使用LLVM工具链和支持的生成器（例如Ninja）配置 `windows/CMakeLists.txt`：

```powershell
cmake -S windows -B build-windows -G Ninja `
  -DCMAKE_C_COMPILER=clang.exe `
  -DCMAKE_CXX_COMPILER=clang++.exe
cmake --build build-windows --parallel
```

#### Web

Web端需要Emscripten：

```bash
emcmake cmake -S web -B build_web -DCMAKE_BUILD_TYPE=Release
cmake --build build_web --parallel
```

`application/assets/fonts` 下的字体会预加载到虚拟 `/assets/fonts` 目录。其他应用资源
会复制到 `build_web/assets`，并通过 `emscripten_fetch` 在运行时按需加载。Web 产物需要
通过 HTTP 服务从 `build_web` 启动，直接使用 `file://` 打开生成的 HTML 将无法按需请求资源。

#### Android

```bash
cd android
./gradlew :app:assembleDebug
```

#### iOS

安装Pod并打开工程前，先生成原生框架：

```bash
./ios/scripts/build_leaf_native_xcframework.sh
cd ios/App
pod install
open App.xcworkspace
```

iOS最低支持版本为15.0。

### 项目架构

```text
.
├── README.md
├── core # C++核心代码
├── ios # iOS端
│   ├── App # 启动模块
│   ├── Leaf_iOS # 平台适配层
│   ├── Leaf_Plugin # 插件接口定义
│   ├── cmake # CMake构建入口
│   └── scripts # 构建脚本
├── android # Android端
│   ├── app # 启动模块
│   ├── leaf-plugin # 插件接口定义
│   └── leaf-android # 平台适配层
├── ohos # 鸿蒙端
│   ├── entry # 启动模块
│   ├── leaf_plugin # 插件接口定义
│   └── leaf_ohos # 平台适配层
├── windows # Windows端
│   └── main.cpp # 平台适配层及入口函数
├── macos # macOS端
│   └── main.cpp # 平台适配层及入口函数
├── web # Web端
│   ├── web_main.cpp # 平台适配层及入口函数
│   └── shell.html # HTML模版
├── application
│   ├── AppLaunch.cpp # App启动入口
│   └── CMakeLists.txt # 配置Demo用到的第三方库、插件
├── examples # Demo工程代码
│   ├── my_profile # 个人资料页
│   └── reader_app # 阅读器App
└── third_party # 第三方依赖库
```

### 插件架构

```text
xxx # 插件名（如：file_picker）
├── CMakeLists.txt # 插件模块加载的CMake文件
├── xxx.h # 对外提供接口的头文件（如：LFFilePicker.h）
├── xxx.cpp # 接口的实现文件（给原生层发消息）
├── ios # iOS端原生层实现（标准CocoaPods依赖模块）
├── android # Android端原生层实现（标准Android依赖模块，可打包为aar）
├── ohos # 鸿蒙端原生层实现（标准鸿蒙依赖模块，可打包为har）
├── windows # Windows端原生层实现
├── macos # macOS端原生层实现
└── web # Web端原生层实现
```
