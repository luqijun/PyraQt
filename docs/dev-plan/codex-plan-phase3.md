# PyraQt Phase 3 开发需求与计划

## Summary

- 将本阶段开发需求与计划写入 `codex-plan-phase3.md`，然后进入 Phase 3 实施。
- Phase 3 交付完整首版：C++ 动态插件框架、Python 命令插件框架、插件管理器 UI、命令注册中心与 Command Palette 全部落地，但都以“可用首版”为目标。
- Python 插件保持子进程命令插件模型，复用现有 Phase 2 的脚本执行链路；不引入进程内 Python 宿主。
- 严格不读取、不引用、不修改 `python-from-qgis-project/`。

## Key Changes

- 构建与目录：
  - 扩展 `CMakeLists.txt`，新增 `src/core/plugin`、`src/core/command`、`src/plugins`、`plugins`、必要的测试与资源文件。
  - 增加 C++ 插件构建宏，输出到应用旁的插件目录，确保本地运行和 CI 构建后都能被 `QPluginLoader` 扫描。
  - 增加至少一个动态 C++ 示例插件和一个 Python 示例插件；Qt5/Qt6 都保持可编译，Qt6 不因插件系统缺失而降级。
- C++ 插件框架：
  - 新增 `IPlugin` 接口、插件元数据约定、`PluginManager`、`PluginRegistry`、`PluginContext`。
  - 使用 `QPluginLoader` 从固定目录扫描动态插件，读取元数据、实例化、初始化、关闭、错误记录和启用状态。
  - C++ 插件首版只开放安全扩展点：注册命令、注册菜单动作、注册工具栏动作、注册面板工厂；不开放直接操纵任意内部 QWidget 的裸指针。
- Python 插件框架：
  - Python 插件目录约定为 `plugins/<plugin_id>/plugin.json + __init__.py`。
  - 首版 Python 插件只负责声明元数据和注册命令；命令执行时通过现有 `ScriptExecutionManager` 启动对应 Python 文件。
  - Python 插件元数据支持：`id`、`name`、`version`、`description`、`entry`、`commands[]`、`enabledByDefault`。
  - 不做进程内常驻 Python 生命周期；“初始化/关闭”语义限定为被插件管理器识别、命令可见性更新与启停状态持久化。
- 命令系统与 Command Palette：
  - 新增 `CommandManager`、`CommandDescriptor`、`CommandAction`、`CommandPaletteDialog`。
  - 统一收集内建动作、脚本动作、C++ 插件命令、Python 插件命令，支持按标题/关键字过滤并执行。
  - Command Palette 首版入口固定为 `Ctrl+Shift+P`，展示命令标题、来源、简短描述和启用状态。
  - 命令系统成为插件和主窗口之间的唯一动作注册入口，避免插件直接篡改主菜单结构。
- 插件管理器 UI：
  - 将现有左侧 `pluginDock` 从占位文本替换为真实插件管理面板。
  - 显示插件名称、类型、版本、描述、启用状态、加载状态、错误信息。
  - 支持启用/禁用切换，并将状态持久化到 `ConfigManager`；首版不做运行时热重载，仅在下次应用初始化时完整生效。
- 应用与主窗口集成：
  - `Application` 增加 `CommandManager`、`PluginManager` 的装配与初始化顺序控制。
  - `MainWindow` 将内建脚本相关动作注册进命令系统，并增加打开 Command Palette 的菜单/快捷键入口。
  - 菜单结构调整为：内建动作仍保留原入口，同时可被 Command Palette 索引；插件动作优先走命令系统，再选择性映射到菜单/工具栏。
- 配置、示例与文档：
  - `ConfigManager` 新增插件状态配置，如 `plugins.disabled_ids`、`plugins.search_paths`、`command_palette.last_query`。
  - 新增示例 C++ 插件：如注册一个消息命令和一个简单工具面板。
  - 新增示例 Python 插件：如注册一个“运行欢迎命令”，通过子进程脚本输出并更新状态栏。
  - 更新 `README.md` 与 `ARCHITECTURE.md`，补充插件目录结构、构建结果位置、启停行为、Command Palette 用法。
