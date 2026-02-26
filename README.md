## Leaf

A Simple Engine for App Development with C++

**author:** Chen Tong

 ### 设计目标

- 设计实现一个跨平台应用开发引擎，实现全平台统一的UI绘制
- 摒弃原生控件，采用自绘UI策略，保证全平台绘制效果的一致
- 底层绘制使用NanoVG库，在OpenGL环境中进行绘制
- 布局使用Yoga库进行计算，纯数学计算，与图形绘制严格解耦

### 迭代计划

#### SDK

##### 高优先级

- [x] LFBox问题修复
- [x] LFButton优化
- [x] LFListView
- [x] LFInput
- [ ] 弹出层
- [ ] 扩展纹理
- [ ] 支持加载网络图片
- [x] 插件机制
- [ ] 局部重绘
- [ ] 纹理清理（释放）
- [x] 适配iOS
- [x] 适配桌面端
- [x] 适配鸿蒙
- [ ] Android setEGLConfigChooser问题修复

##### 低优先级

- [ ] 文字优化
- [ ] 字体加载及打包
- [ ] JS声明式API开发
- [ ] NanoVG替换计划
- [ ] SVG支持
- [ ] LFPageView数据刷新优化（count不变时仍触发视图刷新）

#### Plugin

- [x] 文件选择器（file_picker）
- [x] path_provider

#### Demo

##### 个人信息

- [x] 项目经历下拉展示

##### 阅读App

- [x] 阅读恢复（前后双向加载书籍）
- [x] 翻页回弹问题修复
- [x] 导入书籍
- [ ] 首页优化

##### AI Agent

- [ ] 针对横/竖屏分别布局
- [ ] AI聊天
- [ ] 流式加载对话
- [ ] Session持久化
- [ ] MCP能力（仅针对横屏）

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
├── desktop # 桌面端（Windows/macOS）
│   └── desktop_main.cpp # 平台适配层及入口函数
├── web # Web端
│   ├── web_main.cpp # 平台适配层及入口函数
│   └── shell.html # HTML模版
├── app_adapter
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
├── desktop # 桌面端原生层实现
└── web # Web端原生层实现
```
