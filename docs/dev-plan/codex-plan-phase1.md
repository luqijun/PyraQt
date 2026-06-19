# PyraQt Phase 1 开发需求与计划

## Summary

- 交付范围锁定为 `Phase 1`：可编译运行的 Qt 桌面应用骨架，而不是一次性实现 Python 引擎、插件市场、调试器、自动更新等 Phase 2-4 功能。
- 首先在仓库根目录新增 `codex-plan.md`，写入本开发需求与计划；随后按该计划实现项目。
- 严格不引用、不读取、不修改 `python-from-qgis-project/` 下任何代码，仅保留它作为禁止触碰的外部参考目录。

## Key Implementation

- 使用现代 CMake 架构：顶层 `CMakeLists.txt`、`CMakePresets.json`、`cmake/` 模块、`src/app`、`src/core`、`src/ui`、`resources`、`tests`、`docs`、`.github/workflows`。
- Qt 兼容策略：默认优先构建本机可用的 Qt 5.15，同时通过 `PYRAQT_QT_MAJOR_VERSION=5|6|AUTO` 保留 Qt6 兼容入口。
- 依赖策略：使用 `CPM.cmake + find_package` 混合方式；`spdlog` 默认优先系统包，其次 CPM 拉取，并提供 Qt 日志 fallback，避免无网络环境完全阻塞骨架构建。
- UI 首版使用原生 `QMainWindow + QDockWidget`，实现左侧文件/插件占位面板、中央编辑区占位、底部控制台/日志面板、右侧属性面板，以及布局保存/恢复基础。
- Core 首版实现配置、日志、主题、i18n 四个基础服务：JSON 配置文件 + `QSettings` 会话状态、Light/Dark QSS 热切换、中英文翻译资源加载、日志写入文件和 UI 面板。
- CI 首版提供 GitHub Actions：Linux/macOS/Windows CMake 配置构建、基础测试、clang-format 检查；Qt6 job 保留矩阵但允许通过环境安装 Qt6。
- 文档首版提供 `README.md`、`ARCHITECTURE.md`、`CONTRIBUTING.md`，说明构建、运行、架构边界和后续 Phase 2-4 路线图。

## Public Interfaces

- `Application`：负责启动初始化、服务装配、主窗口创建、退出清理。
- `ConfigManager`：提供基础 `value/setValue/save/load` API，持久化用户配置与会话状态。
- `LogManager`：统一初始化日志 sink，向文件、控制台面板和 Qt message handler 输出日志。
- `ThemeManager`：提供 `setTheme/light/dark/currentTheme`，加载 `resources/themes/*.qss`。
- `I18nManager`：提供 `setLocale/currentLocale/availableLocales`，运行时切换 `zh_CN` 与 `en_US`。
- `MainWindow`：暴露最小服务注入入口，集中管理菜单、工具栏、dock 布局、状态栏与基础动作。

## Test Plan

- CMake 配置测试：验证 `cmake --preset linux-debug-qt-auto` 可生成构建目录。
- 构建测试：验证应用目标、核心库目标、资源目标、测试目标能编译。
- 单元测试：覆盖配置读写、主题名切换、i18n locale 状态、日志初始化不崩溃。
- 运行冒烟测试：应用可启动到主窗口，菜单中可切换 Light/Dark 和中英文，dock 布局可保存恢复。
- CI 验证：至少 Linux Qt5 路径通过；Qt6/Windows/macOS workflow 提供结构化支持。

## Assumptions

- 本阶段不实现 Python 嵌入、脚本编辑器、C++/Python 插件加载、Command Palette、自动更新、崩溃报告，只保留目录、接口占位和路线图。
- 首版 docking 使用 Qt 原生 `QDockWidget`，后续如需更强布局能力再替换为 Qt Advanced Docking System。
- 当前本机只确认 Qt 5.15 可用，因此本地验证以 Qt5 为准；Qt6 兼容代码和 CI 配置会预留。
- 如果执行阶段网络受限导致 CPM 依赖无法下载，会优先使用系统依赖或 fallback，必要时再申请网络权限安装/拉取依赖。
