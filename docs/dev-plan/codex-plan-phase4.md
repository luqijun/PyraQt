# PyraQt Phase 4 开发需求与计划

## Summary

- 将本阶段开发需求与计划写入 `codex-plan-phase4.md`，然后进入 Phase 4 实施。
- Phase 4 交付“发布基础设施首版”：Linux `DEB + ZIP/TGZ` 打包、本地可验证的 CPack 配置、CI 工件上传、自动更新接口与占位 UI、基础崩溃日志与恢复提示、补全发布文档。
- 不集成真实自动更新提供方，也不引入 Crashpad/Breakpad；只把接口、配置、UI、日志与恢复链路搭好。
- 严格不读取、不引用、不修改 `python-from-qgis-project/`。

## Key Changes

- 构建与打包：新增 `cmake/Packaging.cmake` 和 `packaging/` 资源，接入 `install()`、`include(CPack)`、包元数据、许可证/README 打包输入。
- 发布资产与 CI：扩展 GitHub Actions，Linux Qt5 路径生成 CPack 产物并上传 artifacts。
- 自动更新接口与占位 UI：新增 `core/update` 子系统，提供“检查更新”动作、配置项和占位结果。
- 崩溃与恢复基础：新增 `core/runtime` 子系统，写入运行标记、检测上次异常退出、提供 crash log 路径和恢复提示。
- 主窗口与应用集成：`Application` 装配 `UpdateManager` 与 `CrashRecoveryManager`，`MainWindow` 增加检查更新和恢复提示。
- 文档与发布说明：更新 README、架构文档，并新增 `docs/RELEASE.md`。

## Public Interfaces

- `UpdateManager`: `checkForUpdates(manual)`、`currentChannel()`、`setChannel()`、`autoCheckEnabled()`、`setAutoCheckEnabled()`、`lastCheckTime()`。
- `UpdateCheckResult`: `status`、`message`、`latestVersion`、`downloadUrl`。
- `CrashRecoveryManager`: `markAppStarted()`、`markAppExitedNormally()`、`didPreviousRunCrash()`、`crashLogPath()`、`recoveryMessage()`。
- `MainWindow`: 暴露 `Check for Updates` 菜单/工具栏动作，并在检测到异常退出时显示恢复提示。

## Test Plan

- 配置与构建：验证 `cmake --preset linux-debug-qt-auto` 和 `cmake --build --preset linux-debug-qt-auto`。
- 单元测试：覆盖更新检查时间戳写入、占位状态返回、异常退出检测、crash log 写入、正常退出标记恢复。
- 打包测试：验证 `cmake --build --preset linux-debug-qt-auto --target package` 可生成 `.deb`、`.tar.gz`、`.zip`。
- 包内容检查：确认可执行文件、翻译、主题、示例脚本、C++ 插件、Python 插件进入预期目录。
- CI 验证：跨平台构建测试继续保留，Linux Qt5 job 额外生成并上传发布工件。

## Assumptions

- 自动更新在本阶段只提供接口和 UI 占位，不访问网络、不下载安装包、不执行自更新。
- 崩溃恢复只基于干净退出标记和文本日志，不生成 native crash dump。
- Linux 包依赖元数据按 Qt5 路径配置；Qt6 和 Windows/macOS 先保留可扩展的 ZIP/CI 骨架。
- 发布前需要补正式许可证文件、签名策略和真实版本号管理。
