# PyraQt — 通用 Qt 桌面应用模板框架

## 1. 项目概览

项目名称：PyraQt
定位：一个开箱即用的、面向专业开发者的通用 Qt 桌面应用程序模板/框架。
核心价值：现代化构建体系 + 跨平台 + 插件化架构 + 内嵌 Python 脚本引擎。

---

## 2. 技术栈与构建体系

### 2.1 语言与框架
- C++17（最低标准），推荐 C++20
- Qt 5.15.x 和 Qt 6.5+（双版本兼容，通过 CMake 选项切换）
- Python 3.9+（嵌入式解释器，用于脚本引擎）

### 2.2 构建系统
- CMake 3.21+（作为唯一构建系统）
- 使用 CMakePresets.json 管理多平台/多配置预设
- 支持 vcpkg 或 conan 2.x 作为包管理器（可选，通过 CMake 选项启用）
- CPM.cmake 管理仅头文件或轻量依赖

### 2.3 CI/CD
- 提供 GitHub Actions workflow：
  - 矩阵构建：Windows (MSVC 2022) / Linux (GCC 12+, Clang 15+) / macOS (AppleClang)
  - Qt5 与 Qt6 分别构建
  - 自动运行单元测试、静态分析（clang-tidy）、代码格式检查（clang-format）
- 提供 CPack 配置：Windows NSIS/WiX 安装包、Linux AppImage/DEB、macOS DMG

---

## 3. 跨平台支持

### 3.1 目标平台
- Windows 10/11 (x64)
- Linux (Ubuntu 22.04+, Fedora 38+) (x64)
- macOS 12+ (x64 + arm64 Universal Binary)

### 3.2 平台适配要求
- 使用 Qt 抽象层处理原生路径、字体渲染、高 DPI 缩放
- Windows：支持 DPI Per-Monitor Aware V2
- macOS：支持 Dark Mode 与原生菜单栏
- Linux：支持 Wayland 和 X11（运行时自动选择）
- 条件编译通过 CMake 平台判断 + 抽象接口实现，禁止 #ifdef 散落业务代码

---

## 4. 项目架构设计

### 4.1 整体架构
采用分层 + 插件化架构：

```
PyraQt/
├── CMakeLists.txt                 # 顶层 CMake
├── CMakePresets.json
├── cmake/                         # CMake 模块与工具链
│   ├── Qt5Qt6Compat.cmake         # Qt5/Qt6 兼容宏
│   ├── CompilerWarnings.cmake
│   └── Packaging.cmake
├── src/
│   ├── app/                       # 应用入口（main, Application 类）
│   ├── core/                      # 核心库（非 UI，可独立测试）
│   │   ├── plugin/                # 插件系统框架
│   │   ├── config/                # 配置管理（QSettings + JSON schema）
│   │   ├── logging/               # 日志系统（spdlog 集成）
│   │   ├── i18n/                  # 国际化管理器
│   │   ├── scripting/             # Python 脚本引擎核心
│   │   └── command/               # 命令系统（Undo/Redo + Command Palette）
│   ├── ui/                        # UI 层
│   │   ├── mainwindow/            # 主窗口与 Docking 布局
│   │   ├── widgets/               # 自定义控件库
│   │   ├── theme/                 # 主题引擎
│   │   ├── panels/                # 可停靠面板
│   │   │   ├── scripteditor/      # Python 编辑器面板
│   │   │   ├── console/           # 输出控制台面板
│   │   │   ├── filebrowser/       # 文件浏览器面板
│   │   │   ├── properties/        # 属性面板
│   │   │   └── logviewer/         # 日志查看器面板
│   │   └── dialogs/               # 对话框（设置、关于等）
│   └── plugins/                   # 内置插件示例
│       ├── sample_plugin/
│       └── python_tools_plugin/
├── scripts/                       # 内置 Python 示例脚本
├── resources/
│   ├── icons/                     # SVG 图标（支持主题着色）
│   ├── themes/                    # QSS 主题文件
│   ├── translations/              # .ts 翻译文件
│   └── fonts/                     # 内嵌编程字体
├── tests/                         # 单元测试与集成测试
│   ├── unit/
│   └── integration/
├── docs/                          # 文档（Doxygen + Sphinx）
├── packaging/                     # 平台打包资源
└── third_party/                   # 第三方依赖（git submodule 或 CPM）
```

### 4.2 设计原则
- 依赖倒置：core 层不依赖 ui 层，通过信号槽/接口通信
- 插件通过 Qt Plugin 机制（QPluginLoader）+ 自定义 IPlugin 接口加载
- 所有面板均为可停靠（QDockWidget 或 Qt Advanced Docking System）
- 命令模式：所有用户操作封装为 Command 对象，支持 Undo/Redo
- MVVM/MVC 分离：Model 层可脱离 UI 进行单元测试

### 4.3 核心设计模式
- Singleton（Application 上下文）
- Observer（事件总线/消息中心）
- Strategy（主题切换、语言切换）
- Factory（插件注册与创建）
- Command（操作历史）

---

## 5. UI/UX 设计

### 5.1 视觉风格
- 现代 Fluent / Material 混合风格
- 默认提供 Light 和 Dark 两套主题（QSS 实现）
- 支持用户自定义主题（热加载 QSS 文件）
- 图标系统使用 SVG，支持随主题自动变色
- 内嵌等宽编程字体（JetBrains Mono 或 Fira Code，含连字支持）

### 5.2 布局
- 主窗口采用可自由停靠面板布局（类似 VS Code / Qt Creator 风格）
- 左侧：文件浏览器 + 插件面板
- 中央：编辑器区域（支持多 Tab 分栏）
- 底部：控制台输出 + 日志查看器
- 右侧：属性/调试面板
- 支持布局保存与恢复（用户可保存多套布局预设）

### 5.3 交互
- Command Palette（Ctrl+Shift+P）：快速搜索并执行命令
- 全局快捷键管理器（用户可自定义快捷键映射）
- 状态栏显示：当前语言、Python 解释器状态、编码格式、行列号
- 系统托盘支持（可选最小化到托盘）
- 启动界面（Splash Screen）带加载进度

---

## 6. 国际化（i18n）

### 6.1 基本要求
- 使用 Qt Linguist 工具链（lupdate/lrelease）
- 默认支持：简体中文（zh_CN）、英文（en_US）
- 运行时动态切换语言，无需重启应用
- 所有用户可见字符串使用 tr() / QCoreApplication::translate()
- 翻译覆盖：UI 文本、菜单、工具提示、状态栏消息、错误提示

### 6.2 扩展性
- 翻译文件放置于固定目录，支持用户添加新语言包
- 提供翻译贡献指南文档

---

## 7. Python 脚本引擎（核心模块）

### 7.1 嵌入式 Python 解释器
- 使用 pybind11 嵌入 Python 解释器到应用进程内
- 支持指定外部 Python 解释器路径（venv / conda / system）
- Python 执行运行在独立线程/子进程中，避免阻塞 UI
- 提供取消/中止正在运行脚本的能力

### 7.2 脚本编辑器
- 基于 QScintilla 或自研 QPlainTextEdit 子类：
  - Python 语法高亮（关键字、字符串、注释、装饰器等）
  - 自动缩进与智能缩进
  - 代码折叠
  - 行号显示
  - 括号匹配高亮
  - 自动补全（基于 jedi 库的智能补全）
  - 代码片段（Snippets）支持
  - 多文件 Tab 编辑
  - 查找与替换（支持正则表达式）
  - Minimap 代码缩略图（可选）

### 7.3 脚本执行
- 一键运行当前脚本（F5）
- 运行选中代码片段
- 脚本输出重定向到控制台面板（stdout/stderr 分色显示）
- 支持脚本传入参数
- 支持脚本超时设置
- 执行历史记录

### 7.4 脚本调试
- 集成 Python 调试能力：
  - 断点设置/移除（点击行号区域）
  - 单步执行（Step Over / Step Into / Step Out）
  - 变量监视面板（查看当前作用域变量）
  - 调用堆栈显示
  - 条件断点
  - 使用 debugpy 或 bdb 作为底层调试引擎

### 7.5 应用 API 暴露
- 将应用内部功能暴露为 Python API：
  ```python
  import pyra

  # 访问应用窗口
  pyra.app.title = "My App"

  # 操作 UI
  pyra.ui.show_message("Hello from Python!")
  pyra.ui.statusbar.set_text("Script running...")

  # 文件操作
  pyra.fs.read_file("path/to/file")

  # 注册自定义命令
  @pyra.command("my_command")
  def my_command():
      print("Custom command executed!")

  # 注册菜单项
  pyra.menu.add_item("Tools/My Tool", my_command)

  # 事件钩子
  @pyra.on("app.startup")
  def on_startup():
      print("App started!")
  ```
- API 文档自动生成

### 7.6 包管理
- 内置 pip 集成界面（安装/卸载/列出 Python 包）
- 支持 requirements.txt 一键安装
- 虚拟环境管理（创建/切换/删除 venv）

---

## 8. 插件系统

### 8.1 C++ 插件
- 基于 Qt Plugin 机制
- 定义 IPlugin 接口：
  ```cpp
  class IPlugin {
  public:
      virtual ~IPlugin() = default;
      virtual QString id() const = 0;
      virtual QString name() const = 0;
      virtual QString version() const = 0;
      virtual bool initialize(IApplication* app) = 0;
      virtual void shutdown() = 0;
  };
  ```
- 插件可注册：菜单项、工具栏按钮、面板、命令、设置页面

### 8.2 Python 插件
- Python 脚本也可作为插件加载
- 约定目录结构：
  ```
  plugins/
  └── my_plugin/
      ├── plugin.json          # 元数据
      └── __init__.py          # 入口
  ```
- 插件生命周期管理（启用/禁用/重新加载）

### 8.3 插件管理器 UI
- 列出已安装插件（名称、版本、作者、描述）
- 启用/禁用开关
- 在线插件仓库浏览（可选，后期扩展）

---

## 9. 配置管理

- 分层配置：默认值 → 系统级 → 用户级 → 项目级
- 配置格式：JSON（带 schema 校验）
- 设置对话框：分类树 + 搜索 + 实时预览
- 支持导入/导出配置
- 敏感配置加密存储（如 API Key）

---

## 10. 日志系统

- 集成 spdlog（高性能日志）
- 日志级别：Trace / Debug / Info / Warn / Error / Fatal
- 输出目标：文件（按日期轮转）、控制台面板、系统日志
- UI 日志查看器面板：
  - 按级别过滤/着色
  - 实时搜索
  - 时间戳显示
  - 导出日志

---

## 11. 其他功能

### 11.1 自动更新
- 集成 Qt Installer Framework 或 Sparkle (macOS) / WinSparkle (Windows)
- 启动时检查更新（可配置频率）
- 支持增量更新（可选）

### 11.2 崩溃报告
- 集成 Breakpad 或 Crashpad
- 崩溃时生成 minidump
- 提示用户上报崩溃信息

### 11.3 会话管理
- 记住上次窗口位置/大小/面板布局
- 记住上次打开的文件/Tab
- 最近文件列表

### 11.4 性能
- 启动时间优化：延迟加载插件、懒初始化面板
- 大文件处理：编辑器支持增量加载
- 内存监控：状态栏可显示当前内存占用

### 11.5 无障碍（Accessibility）
- 所有控件设置 AccessibleName 和 AccessibleDescription
- 支持键盘完全操作（Tab 导航、快捷键）
- 支持屏幕阅读器

---

## 12. 测试要求

### 12.1 单元测试
- 框架：Qt Test + Google Test（双框架支持）
- 覆盖 core 层所有公共 API
- Mock 框架：GMock

### 12.2 集成测试
- 测试插件加载/卸载
- 测试 Python 脚本执行与 API 调用
- 测试 i18n 切换

### 12.3 UI 测试
- 使用 Qt Test 的 QTest::mouseClick 等进行基本 UI 自动化
- Squish 兼容（可选）

### 12.4 覆盖率
- 集成 gcov/lcov（Linux）或 OpenCppCoverage（Windows）
- CI 中生成覆盖率报告

---

## 13. 文档

- README.md：项目介绍、快速开始、截图
- CONTRIBUTING.md：贡献指南
- ARCHITECTURE.md：架构设计文档
- API 文档：Doxygen 生成 C++ API 文档
- Python API 文档：Sphinx 生成
- 用户手册：基础使用指南（Markdown）

---

## 14. 代码规范

- C++ 代码风格：基于 Google C++ Style Guide，通过 .clang-format 强制
- 命名约定：
  - 类名：PascalCase
  - 函数/方法：camelCase
  - 成员变量：m_camelCase
  - 常量/枚举：UPPER_SNAKE_CASE
- 文件命名：snake_case.cpp / snake_case.h
- 每个公共头文件包含 Doxygen 注释
- commit 消息遵循 Conventional Commits 规范

---

## 15. 开发优先级（分阶段交付）

### Phase 1 — 骨架（MVP）
1. CMake 构建系统搭建（Qt5/Qt6 兼容）
2. 主窗口 + Docking 框架
3. 主题系统（Light/Dark 切换）
4. 配置管理基础
5. 日志系统集成
6. 中英文 i18n 基础

### Phase 2 — Python 引擎
7. 嵌入 Python 解释器
8. 脚本编辑器（语法高亮、补全）
9. 脚本执行 + 控制台输出
10. 基础调试功能（断点、单步）
11. Python API 暴露

### Phase 3 — 插件与扩展
12. C++ 插件框架
13. Python 插件框架
14. 插件管理器 UI
15. Command Palette
16. 快捷键管理

### Phase 4 — 完善与发布
17. 自动更新
18. 崩溃报告
19. 完善测试与 CI/CD
20. 打包与安装程序
21. 文档完善

---

## 16. 立即开始

请按照 Phase 1 开始实施。首先：
1. 创建完整的项目目录结构
2. 编写顶层 CMakeLists.txt 与 CMakePresets.json
3. 实现 Qt5/Qt6 兼容层（cmake/Qt5Qt6Compat.cmake）
4. 实现 Application 类与 MainWindow（含 Docking 布局）
5. 集成 spdlog 日志
6. 实现主题引擎（Light/Dark QSS 切换）
7. 实现 i18n 管理器与中英文翻译文件
8. 实现基础配置管理（JSON + QSettings）
9. 编写 GitHub Actions CI workflow
10. 编写 README.md

要求所有代码可以直接编译通过，提供清晰的构建说明。
