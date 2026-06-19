# PyraQt Phase 2 开发需求与计划

## Summary

- 将本计划与最终开发需求写入 `codex-plan-phase2.md`，然后进入 Phase 2 实施。
- Phase 2 交付目标锁定为“可用脚本链路”：Qt5 下提供可运行的 Python 脚本编辑与执行体验；Qt6 保持可编译，但脚本编辑器降级为占位说明。
- 不实现完整调试器、Jedi 补全、包管理、venv 管理；这些只保留架构接口和后续扩展位。
- 严格不读取、不引用、不修改 `python-from-qgis-project/`。

## Key Changes

- 构建与依赖：
  - 扩展 CMake，增加 `Python3::Interpreter` / `Python3::Module` 或等效嵌入检测、`python3-embed` 支持、QScintilla Qt5 检测逻辑。
  - Qt5 下启用 QScintilla 编辑器目标；Qt6 或未找到 QScintilla 时编译占位脚本面板，不让整体构建失败。
  - 为 Phase 2 增加示例脚本资源与测试目标，CI 至少验证 Linux Qt5 脚本链路可编译。
- 脚本核心：
  - 新增 `core/scripting` 子系统，包含 `PythonRuntimeManager`、`ScriptExecutionManager`、`ScriptProcessRunner`、`PyraApiBridge`、`ScriptSession`。
  - 运行模型固定为独立 Python 进程，通过 `QProcess` 执行当前脚本或临时片段文件，支持开始、停止、超时、退出码、stdout/stderr 分流。
  - 解释器来源优先使用配置中的解释器路径，否则回退系统 `python3`。
  - `PyraApiBridge` 以启动脚本注入方式提供最小 `pyra` API，不做进程内嵌入解释器。
- 编辑器与 UI：
  - Qt5 下新增 QScintilla 脚本编辑面板，具备 Python lexer、行号、当前行高亮、基础自动缩进、打开/保存、新建、轻量单文档编辑体验。
  - 中央编辑区由占位 `QTextEdit` 替换为脚本编辑工作区；底部控制台复用现有输出面板并区分 stdout/stderr。
  - 菜单和工具栏新增 `New Script`、`Open Script`、`Save`、`Run`、`Stop` 动作，以及状态栏 Python 解释器状态更新。
  - Qt6 下脚本编辑区显示“Phase 2 editor unavailable on this build”说明，但保留运行服务接口和状态展示。
- 最小 Python API：
  - `pyra.app`: 只读应用名、版本、当前解释器路径。
  - `pyra.ui.show_message(text)`：触发 Qt 消息提示。
  - `pyra.ui.set_status(text)`：更新状态栏文本。
  - `pyra.fs.read_text(path)` / `write_text(path, text)`：基础文本文件访问。
  - `pyra.log.info/warn/error(text)`：写入应用日志与控制台。
  - 不开放任意 QWidget 指针、菜单动态注入、窗口标题写入等高耦合能力。
- 配置与示例：
  - `ConfigManager` 增加脚本相关配置：解释器路径、执行超时、最近脚本、最近目录。
  - `scripts/` 下提供 2-3 个首版示例脚本，覆盖消息提示、状态栏更新、文件读写。
  - 文档更新 `README.md` 与架构文档，补充 Phase 2 构建要求、Qt5/Qt6 差异、API 示例。

## Public Interfaces

- `PythonRuntimeManager`
  - `interpreterPath()`, `setInterpreterPath()`, `isInterpreterAvailable()`, `pythonVersion()`
- `ScriptExecutionManager`
  - `runFile(path, args)`, `runSelection(code, fileContext)`, `stop()`, `isRunning()`
  - 信号：`executionStarted`, `stdoutReceived`, `stderrReceived`, `executionFinished`, `executionFailed`
- `PyraApiBridge`
  - 生成注入脚本与运行时辅助文件，保证子进程内可 `import pyra`
- `ScriptEditorWidget`
  - 封装 QScintilla 编辑器、当前文件路径、dirty 状态、加载/保存/选区读取
- `MainWindow`
  - 增加脚本相关动作绑定、控制台联动、解释器状态展示与脚本工作区装配

## Test Plan

- 构建测试：
  - Qt5 + QScintilla 环境下可完整配置和构建。
  - Qt6 或无 QScintilla 环境下仍可配置和构建，占位脚本面板生效。
- 单元测试：
  - 解释器探测、脚本命令组装、超时/取消状态流转、`pyra` 注入脚本生成、配置读写。
- 集成测试：
  - 运行示例脚本可捕获 stdout/stderr。
  - `pyra.ui.set_status` 和 `pyra.log.info` 能回流到应用状态栏/日志通道。
  - 停止长时间脚本可结束子进程并恢复 UI 状态。
- UI 冒烟测试：
  - 新建/打开/保存脚本、运行当前脚本、停止脚本、控制台显示输出。
  - Qt6 构建下脚本面板正确显示降级说明，不触发崩溃。

## Assumptions

- `codex-plan-phase2.md` 的内容以本计划为准，实施前先写入仓库根目录。
- Phase 2 默认以 Qt5 为完整功能目标，因为本机已确认 QScintilla Qt5 头文件和库可用；Qt6 只保证可编译与占位降级。
- 调试、补全、断点、pip/venv 管理不在本次实现范围内，只预留后续接口。
- Python 执行采用子进程模型而非进程内嵌入，因此“停止脚本”语义为终止子进程，不保证脚本内清理回调执行。
- 首版编辑器优先单文档或轻量标签体验，不引入复杂项目工作区模型。
