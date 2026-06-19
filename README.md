# PyraQt

PyraQt 是一个面向专业开发者的现代 Qt 桌面应用模板框架。当前仓库实现了 `Phase 5` 首版：在脚本、命令、插件和发布基础设施之上，补齐单窗口多标签脚本工作区、最近文件、会话恢复、真实文件浏览器和应用设置对话框。

## 当前能力

- `QMainWindow + QDockWidget` 桌面骨架
- Qt5 + QScintilla 脚本编辑器
- 单窗口多标签脚本工作区
- 最近文件、打开文件会话保存与恢复
- 本地文件浏览器面板
- 应用级设置对话框
- Python 子进程执行与停止
- 最小 `pyra` API：`app`、`ui`、`fs`、`log`
- C++ 动态插件与 Python 命令插件
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
- `resources`: QSS 主题与翻译资源
- `tests`: QtTest 单元测试

更详细设计见 [docs/ARCHITECTURE.md](/home/numbat/Projects/Misc/PyraQt/docs/ARCHITECTURE.md)。

## 路线图

- Phase 6: 查找替换、脚本运行参数/历史、快捷键管理和更完整脚本 IDE 体验
- Phase 7: 真实更新源、崩溃报告后端、签名发布和更完整安装器

## 约束

- 不引用、不读取、不修改 `python-from-qgis-project/` 内任何代码。
