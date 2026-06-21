# Architecture

## Phase 5 目标

当前版本在可用脚本、插件、命令系统和发布基础设施基础上，补齐单窗口多标签工作区、最近文件、会话恢复、真实文件浏览器和应用设置对话框。

## 分层设计

- `app`: 启动、生命周期、服务装配
- `core`: 与 UI 解耦的基础服务
- `ui`: 主窗口、dock、菜单、状态栏

当前 `Application` 在启动时装配 `ConfigManager`、`LogManager`、`ThemeManager`、`I18nManager`、`PythonRuntimeManager`、`PyraApiBridge`、`ScriptExecutionManager`、`CommandManager`、`PluginManager`、`UpdateManager`、`CrashRecoveryManager` 和 `WorkspaceManager`，并在退出时统一保存窗口布局与工作区会话。
当构建启用 OCCT 时，还会装配 `ModelImportManager` 用于 STEP/BREP 导入，并由 UI 层创建真实 `V3d/AIS` 视口用于模型浏览。

## 核心服务

- `ConfigManager`: JSON 文件保存应用偏好，`QSettings` 保存窗口几何和布局状态。
- `LogManager`: 统一处理日志输出，默认使用 Qt 文件日志；检测到 `spdlog` 时可启用增强输出。
- `ModelImportManager`: 检测并读取 `.stp/.step/.brep` 文件，构造可长期持有的模型文档、统计基础拓扑摘要并提取基础几何量；无 OCCT 时返回禁用提示。
- `ThemeManager`: 通过 QSS 在 Light / Dark 主题间切换。
- `I18nManager`: 加载 `translations/` 下的 `.qm` 文件，实现运行时语言切换。
- `PythonRuntimeManager`: 对齐 QGIS `QgsPythonUtils` 的主进程 CPython 托管层，负责解释器初始化、`sys.path`、共享 globals、GIL 封装、traceback、stdout/stderr 重定向和 Python 安全/执行配置。
- `PythonRunner`: 对齐 QGIS `QgsPythonRunner` 的统一执行入口，提供 `exec`、`eval`、文件执行、单行命令和函数回调。
- `PythonFeatureManager`: 提供项目宏、表达式函数和 Processing 风格算法的最小完整入口。
- `PyraApiBridge`: 生成子进程可导入的 `pyra` 模块与 bootstrap 脚本。
- `ScriptExecutionManager`: 默认在内嵌 CPython 中执行脚本，并保留 `QProcess` 隔离执行用于长任务或兼容场景。
- `CommandManager`: 统一注册与执行内建命令、C++ 插件命令和 Python 插件命令。
- `PluginManager`: 扫描应用旁和配置项里的插件目录、去重收集插件、校验依赖、加载/卸载动态 C++ 与 Python 插件，并持久化启停状态。
- `UpdateManager`: 提供检查更新、渠道和自动检查配置。当前只返回开发构建的 `NotConfigured` 占位结果，不访问网络。
- `CrashRecoveryManager`: 在启动时标记运行中，在正常退出时标记干净退出；下次启动发现异常退出会写入 `crash.log` 并提示恢复信息。
- `WorkspaceManager`: 管理最近文件、打开文件会话、活动文件和文件浏览器根目录，恢复时过滤不存在的文件。

## UI Shell

主窗口使用原生 `QDockWidget`，左侧包含本地文件浏览器和插件管理器，底部提供 QGIS 风格 Python Console 与日志视图，中央区域为标签式文档工作区：脚本文件进入编辑器标签，模型文件进入 OCCT 3D 视图标签，并提供 Command Palette 作为统一命令入口。右侧全局 `Properties` dock 根据当前活动上下文显示脚本占位、模型整体属性或当前单选子形属性。`Tools > Python Tools` 提供宏、表达式函数和 Processing 风格算法的图形管理入口。

## Python 嵌入架构

PyraQt 的 Python 子系统采用 QGIS 等价设计：Qt 主进程内全局单例 CPython 解释器、共享 `__main__` globals、统一 Runner 获取/释放 GIL，并通过内建 `pyra` 包与全局 `iface` 暴露稳定 API。插件、Console、脚本、宏、表达式函数和 Processing 风格算法共享同一解释器环境，因此变量、导入和插件状态可互通。

Python 插件支持 `plugin.json` 和 QGIS 风格 `metadata.txt`。加载流程为扫描插件目录、校验依赖、按 `entry` 创建独立运行时模块、调用 `classFactory(iface)`、调用插件对象的 `initGui()`，并注册清单命令或运行期动态命令。插件卸载时会调用 `unload()`，同时清理由该插件注册的命令、表达式函数和 Processing 算法。`ScriptProcessRunner` 仍保留为隔离执行路径，用于绕开 GIL 或避免长脚本阻塞主进程。

`pyra.commands.register()`、`pyra.macros.load()/trigger()`、`pyra.expressions.register()/evaluate()` 和 `pyra.processing.register()/run()` 会通过 native bridge 回到 C++ 服务层。全局 `iface` 映射到这些 PyraQt API 入口，提供类似 QGIS `qgis.utils.iface` 的 GUI/命令访问面。项目宏默认关闭，需要在设置中显式授权；普通脚本默认内嵌执行，也可切换为隔离子进程执行；`pyra.fs` 文件读写受设置中的文件系统访问开关约束。
Python Console 提供命令输入、多行执行区、历史记录、stdout/stderr 回流和简版对象检查，所有 Console 命令都在共享解释器中运行。
代码提示由统一的 `PythonCompletionProvider` 提供词源。QScintilla 构建下脚本编辑器使用原生 API 自动补全；Console 的单行输入和多行区使用 Qt `QCompleter`，并在脚本执行后刷新共享解释器中的 globals。

## 工作区与设置

工作区会话只保存已落盘文件路径、当前活动文件、最近文件和文件浏览器根目录，不保存未保存缓冲区。会话恢复不区分脚本和模型路径，按扩展名重新路由。设置对话框提供应用级选项，包括主题、语言、Python 解释器、执行超时、启动恢复、文件浏览根目录和更新检查配置；项目级配置、快捷键自定义和敏感配置加密留给后续阶段。

## 模型导入边界

- 当前支持格式：`.stp`、`.step`、`.brep`
- 当前提供只读 3D 浏览器，支持旋转、平移、缩放、Fit All、标准视角、线框/着色/带边着色、边/面/实体/点选择和 hover 高亮
- 当前属性面板显示包围盒、拓扑统计以及长度/面积/体积等基础几何量
- 当前不提供装配树、颜色/图层解析、隐藏/隔离、多选聚合、编辑与导出

## 发布管线

`cmake/Packaging.cmake` 集中维护安装规则和 CPack 元数据。Linux 默认生成 `DEB`、`TGZ` 和 `ZIP`；其他平台保留 `ZIP` 包结构。GitHub Actions 在 Linux Qt5 job 中执行 `package` 目标并上传产物，跨平台 job 继续负责配置、编译和测试。

## 后续扩展点

- 真实更新源、签名校验和差分更新。
- Crashpad/Breakpad 或平台原生日志收集。
- 发布签名、公证、安装器定制和自动化版本号管理。
- 查找替换、运行参数/历史、调试器、包管理和快捷键自定义。
