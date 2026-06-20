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
- `PythonRuntimeManager`: 解析解释器路径、版本和运行超时配置。
- `PyraApiBridge`: 生成子进程可导入的 `pyra` 模块与 bootstrap 脚本。
- `ScriptExecutionManager`: 通过 `QProcess` 运行脚本并把 stdout/stderr 与桥接命令回流到 UI。
- `CommandManager`: 统一注册与执行内建命令、C++ 插件命令和 Python 插件命令。
- `PluginManager`: 扫描应用旁的插件目录、加载动态 C++ 插件、解析 Python 插件清单并持久化启停状态。
- `UpdateManager`: 提供检查更新、渠道和自动检查配置。当前只返回开发构建的 `NotConfigured` 占位结果，不访问网络。
- `CrashRecoveryManager`: 在启动时标记运行中，在正常退出时标记干净退出；下次启动发现异常退出会写入 `crash.log` 并提示恢复信息。
- `WorkspaceManager`: 管理最近文件、打开文件会话、活动文件和文件浏览器根目录，恢复时过滤不存在的文件。

## UI Shell

主窗口使用原生 `QDockWidget`，左侧包含本地文件浏览器和插件管理器，底部保留控制台和日志，中央区域为标签式文档工作区：脚本文件进入编辑器标签，模型文件进入 OCCT 3D 视图标签，并提供 Command Palette 作为统一命令入口。右侧全局 `Properties` dock 根据当前活动上下文显示脚本占位、模型整体属性或当前单选子形属性。

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
