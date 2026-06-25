# PyraQt

PyraQt 是一个面向专业开发者的现代 Qt 桌面应用模板框架。当前仓库实现了 `Phase 5` 首版：在脚本、命令、插件和发布基础设施之上，补齐单窗口多标签脚本工作区、最近文件、会话恢复、真实文件浏览器和应用设置对话框。

当前还额外提供 `Phase 7` 首版模型浏览能力：在检测到本机预装 OCCT 时，可打开 `.stp/.step/.brep` 文件，在中央标签页显示真实 3D 模型，并在右侧属性面板显示模型或当前选中子形的属性。

## 当前能力

- `QMainWindow + QDockWidget` 桌面骨架
- Qt5 + QScintilla 脚本编辑器
- 单窗口多标签脚本工作区
- 最近文件、打开文件会话保存与恢复
- 本地文件浏览器面板
- 应用级设置对话框
- 可选 OCCT 3D 模型浏览器（`.stp/.step/.brep`）
- QGIS 风格主进程内嵌 CPython 运行时、共享全局命名空间和统一 Runner
- 可选 Python 子进程隔离执行与停止
- 内嵌 `pyra` / `iface` API：`app`、`ui`、`fs`、`log`、`commands`、`workspace`、`cad`、`plugins`、`processing`、`macros`、`expressions`
- C++ 动态插件与 QGIS 风格 `classFactory(iface)` Python 插件
- 交互式 Python Console、stdout/stderr 回流、命令历史和多行执行区
- Python Tools 对话框，用于加载/触发宏、注册/执行表达式函数和 Processing 风格算法
- Python 脚本编辑器与 Console 代码提示，覆盖 Python 关键字、`pyra`/`iface` API 和运行时 globals
- Python 宏、表达式函数和 Processing 风格算法入口
- Command Palette 与插件管理器
- CPack 发布打包：Linux `DEB`、`TGZ`、`ZIP`
- 自动更新检查占位动作与配置
- 异常退出检测、故障日志与恢复提示
- Light / Dark 主题切换
- 中英文运行时切换
- JSON + `QSettings` 基础配置持久化
- 文件日志 + UI 控制台镜像
- CMake Presets、CTest、QtTest

## 快速开始

### 依赖

- CMake 3.21+
- Ninja
- Qt 5.15+ 或 Qt 6.5+
- C++17 编译器
- Qt5 完整脚本编辑体验需要 QScintilla 开发库
- 嵌入式 Python 需要 Python3 Development 组件
- 如果需要 STEP/BREP 导入与 3D 浏览功能，需本机预装 OCCT 开发库

### 本地构建

```bash
cmake --preset linux-debug-qt-auto
cmake --build --preset linux-debug-qt-auto
ctest --preset linux-debug-qt-auto
```

运行程序：

```bash
./build/linux-debug-qt-auto/pyraqt
```

示例脚本位于 [scripts](/home/numbat/Projects/Misc/PyraQt/scripts)。
Python 插件示例位于 [plugins](/home/numbat/Projects/Misc/PyraQt/plugins)。
Python 插件推荐实现 `classFactory(iface)`，返回带 `initGui()` / `unload()` 的插件对象；旧的 `plugin.json` 命令式插件继续兼容。
当前插件管理器支持：

- 扫描应用旁插件目录和 `plugins.search_paths` 里的额外目录，并自动去重
- Python / C++ 插件启用、禁用、重新加载和依赖校验
- Python 插件在 `initGui()` 中动态注册命令、表达式函数和 Processing 算法，并在卸载时按插件归属清理

仓库内新增了 3 个不同方向的 Python 插件示例：

- `runtime_command_plugin`：演示 `initGui()` 动态注册命令和生命周期日志
- `analysis_tools_plugin`：演示表达式函数与 Processing 算法注册
- `dependent_workflow_plugin`：演示插件依赖、运行期命令协作和 `plugin.json` 脚本命令

Console 中可直接使用 `pyra.macros`、`pyra.expressions` 和 `pyra.processing` 注册并调用 PyraQt 领域的宏、表达式函数和 Processing 风格算法，也可以通过全局 `iface` 访问同等入口。
菜单 `Tools > Python Tools` 提供同等图形入口；Python Console 的 `Inspect` 按钮可查看当前共享解释器全局对象。
脚本编辑器和 Python Console 默认启用代码提示；可在 Settings 的 Python 页面调整启用状态和触发字符数。

如果需要强制选择 Qt 版本：

```bash
cmake --preset qt5-debug
cmake --preset qt6-debug
```

### 本地打包

```bash
cmake --preset linux-release-qt-auto
cmake --build --preset linux-release-qt-auto
cmake --build --preset linux-release-qt-auto --target package
```

Linux 下会在构建目录生成 `.deb`、`.tar.gz` 和 `.zip`。CI 的 Linux Qt5 job 也会上传这些包作为 workflow artifacts。

## 架构概览

- `src/app`: 应用启动与服务装配
- `src/core`: 配置、日志、主题、国际化
- `src/core/update`: 更新检查接口与占位实现
- `src/core/runtime`: 运行状态、异常退出检测与恢复提示
- `src/core/workspace`: 最近文件、工作区会话与文件浏览根目录
- `src/ui`: 主窗口和停靠面板
- `src/core/cad`: OCCT 文件导入、CAD 文档统计与属性提取
- `resources`: QSS 主题与翻译资源
- `tests`: QtTest 单元测试

更详细设计见 [docs/ARCHITECTURE.md](/home/numbat/Projects/Misc/PyraQt/docs/ARCHITECTURE.md)。

## 模型导入说明

- 当前支持的模型格式仅有 `.stp`、`.step`、`.brep`
- 当前能力包含导入、3D 浏览和属性查看，不包含建模编辑
- 当前支持旋转、平移、缩放、Fit All、标准视角、线框/着色/带边着色、边/面/实体/点选择和单选属性查看
- 如果 CMake 配置阶段未检测到 OCCT，应用仍可构建运行，但模型导入功能会禁用并在打开时给出提示
