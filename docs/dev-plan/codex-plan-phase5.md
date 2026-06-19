# PyraQt Phase 5 开发计划：工作区与设置首版

## Summary

- 将本阶段最终开发需求与计划写入 `docs/dev-plan/codex-plan-phase5.md`。
- Phase 5 聚焦把当前 Phase 4 骨架提升为“可日常使用的单窗口脚本工作区”，优先补齐多标签编辑、最近文件、会话恢复、真实文件浏览器和应用设置对话框。
- 本阶段明确不引入项目/workspace 文件格式，不实现项目级配置、调试器、包管理、真实自动更新、真实崩溃上报、快捷键自定义和完整 IDE 能力。
- 严格不读取、不引用、不修改 `python-from-qgis-project/`。

## Key Changes

- 工作区与会话：
  - 将中央区域从单一 `ScriptEditorWidget` 升级为标签式脚本工作区，支持 `New/Open/Save/Save All/Close/Close Others`。
  - 增加最近文件列表、上次打开标签恢复、当前活动标签恢复、未保存标签关闭确认。
  - 会话恢复仅基于“文件路径列表 + 当前活动文件 + 窗口布局”，不保存未落盘缓冲区内容。
  - `MainWindow` 补充 `Recent Files` 子菜单、`Open Recent`、`Reopen Last Session`、`Close Tab` 等内建命令并注册到 Command Palette。
- 编辑器与文件浏览：
  - 新增工作区容器组件，统一管理多个 `ScriptEditorWidget` 实例、dirty 状态、文件路径和标签标题。
  - 左侧 `File Browser` 从占位面板替换为真实文件树，首版基于本地目录浏览与双击打开，不引入项目模型。
  - 文件浏览器根目录默认使用最近目录；支持从菜单选择根目录并持久化。
  - Qt5 下继续使用 QScintilla；Qt6/无 QScintilla 时仍保留降级编辑器占位，但工作区、最近文件、文件树和会话逻辑照常工作。
- 设置系统：
  - 新增应用级设置对话框，采用“分类列表 + 右侧表单 + 搜索过滤”的首版结构。
  - 首版设置项覆盖：主题、语言、Python 解释器路径、执行超时、启动时恢复上次会话、默认文件浏览器目录、自动检查更新开关与更新渠道。
  - 设置修改即时写入 `ConfigManager` 并尽量实时生效；不能即时生效的项在 UI 中明确提示下次启动生效。
  - 不做项目级设置、schema 校验器、敏感信息加密、配置导入导出。
- 配置与服务：
  - 扩展 `ConfigManager` 默认键，新增 `workspace.recent_files`、`workspace.open_files`、`workspace.active_file`、`workspace.restore_last_session`、`workspace.file_browser_root`、`workspace.max_recent_files`。
  - 新增轻量工作区/会话服务，负责最近文件去重、数量裁剪、可恢复文件过滤和标签状态序列化。
  - `Application` 启动顺序调整为：先载入配置，再恢复工作区/窗口会话，再初始化主窗口内容；关闭时统一保存布局和工作区状态。
- UI 细化与文档：
  - 状态栏补充当前文件路径简写、行列号和 dirty 状态；无可用编辑器时显示清晰降级信息。
  - 为主要可交互控件补 `accessibleName`/`accessibleDescription` 的首版覆盖，至少覆盖主菜单动作、工作区标签、文件树、设置对话框核心控件。
  - 更新 `README.md`、`docs/ARCHITECTURE.md`，新增工作区与设置章节，说明 Phase 5 范围和未覆盖项。

## Public Interfaces

- `WorkspaceManager` 或等效核心服务：
  - `openFile(path)`, `newDocument()`, `closeDocument(id)`, `saveDocument(id)`, `saveAll()`
  - `recentFiles()`, `addRecentFile(path)`, `restoreSession()`, `captureSession()`
- `WorkspaceSession`：
  - 至少包含 `openFiles`, `activeFile`, `recentFiles`, `fileBrowserRoot`
- `EditorWorkspaceWidget`：
  - 封装标签容器与多个 `ScriptEditorWidget`
  - `openFile(path)`, `newDocument()`, `currentEditor()`, `saveCurrent()`, `saveAll()`, `closeCurrent()`
- `SettingsDialog`：
  - 暴露最小注入入口，接收 `ConfigManager`、`ThemeManager`、`I18nManager`、`PythonRuntimeManager`、`UpdateManager`
- `MainWindow`：
  - 改为围绕工作区容器而非单编辑器工作
  - 新增设置、最近文件、恢复会话、关闭标签、保存全部等动作与命令注册

## Test Plan

- 构建测试：
  - Qt5 路径完整配置与构建通过。
  - Qt6 或无 QScintilla 路径仍可配置与构建，工作区容器与设置对话框可编译。
- 单元测试：
  - 最近文件去重与数量裁剪。
  - 会话序列化/反序列化与不存在文件过滤。
  - 新增配置项默认值与读写。
  - 设置修改后主题/语言/解释器路径状态同步。
- 集成测试：
  - 打开多个脚本后保存会话，重启可恢复标签与活动文件。
  - 文件浏览器双击文件可打开到新标签。
  - `Save All`、关闭脏标签确认、最近文件菜单联动正常。
  - 设置对话框修改主题/语言后 UI 立即更新；解释器路径修改后运行脚本使用新路径。
- UI 冒烟测试：
  - 新建、打开、关闭、切换多个标签可用。
  - `Recent Files`、`Open Recent`、`Reopen Last Session`、设置对话框可从菜单和 Command Palette 触发。
  - 状态栏显示文件状态与行列号。
  - Qt6 降级编辑器提示仍正确显示，不崩溃。

## Assumptions

- `docs/dev-plan/codex-plan-phase5.md` 以本计划为准。
- Phase 5 只做“单窗口多标签工作区”，不引入项目根目录文件格式或项目级配置覆盖链。
- 未保存的新建文档不会参与跨重启内容恢复；只恢复已落盘文件。
- 快捷键自定义、查找替换、调试器、包管理、托盘、Splash、真实更新和真实崩溃报告继续留给后续阶段。
- 文件浏览器首版基于 Qt 文件系统模型实现本地浏览，不加入监视器高级功能、收藏夹或远程文件系统。
