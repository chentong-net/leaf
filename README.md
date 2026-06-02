## Leaf

**A Cross-Platform App Development Engine in C++**

**Author:** Chen Tong

English | [中文](README-zh.md)

### Design Goals

- Build a cross-platform app engine with unified UI rendering across all platforms.
- Avoid native controls and use fully custom-drawn UI to ensure consistent visual output.
- Use NanoVG for low-level rendering in an OpenGL environment.
- Use Yoga for layout calculation, keeping pure layout math strictly decoupled from rendering.

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
├── desktop # Desktop (Windows/macOS)
│   └── desktop_main.cpp # platform adapter layer and entrypoint
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
├── desktop # Desktop native implementation
└── web # Web native implementation
```

### Roadmap

#### SDK

##### High Priority

- [x] LFBox fixes
- [x] LFButton improvements
- [x] LFListView
- [x] LFInput
- [ ] Overlay / popup layer
    - [x] Simple overlay
    - [x] Dropdown
- [ ] Plugin resource merging/packaging script
- [ ] External texture support
- [ ] Network image loading support
- [x] Plugin mechanism
- [ ] Partial repaint
- [ ] Texture cleanup (release)
- [x] iOS support
- [x] Desktop support
- [x] HarmonyOS support
- [ ] Fix Android `setEGLConfigChooser` issue

##### Low Priority

- [ ] Text rendering improvements
- [ ] Font loading and packaging
- [ ] Declarative JS API
- [ ] NanoVG replacement plan
- [ ] SVG support
- [ ] LFPageView data refresh optimization (avoid unnecessary view refresh when count is unchanged)

#### Plugins

- [x] File picker (`file_picker`)
- [x] path_provider

#### Demos

##### Profile

- [x] Expand/collapse project experience section

##### Reader App

- [x] Reading state restore (bidirectional pre/next book loading)
- [x] Page-turn rebound fix
- [x] Book import
- [ ] Home page optimization

##### AI Agent

- [ ] Separate layouts for landscape/portrait
- [ ] AI chat
- [ ] Streaming responses
- [ ] Session persistence
- [ ] MCP capabilities (landscape only)
