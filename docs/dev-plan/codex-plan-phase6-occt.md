# PyraQt Phase 6 开发计划：OCCT 最小模型导入能力

## Summary

- 本阶段目标限定为“最小可用导入链路”：应用可打开 `.stp/.step/.brep` 文件，成功时在中心标签页显示只读模型摘要信息，不做 3D 视口、不做编辑、不做装配树浏览。
- OCCT 采用“本机预装 + 构建时检测”策略：主线项目继续可构建；检测到 OCCT 时启用模型导入，未检测到时模型功能禁用并给出明确提示。
- 现有脚本工作区、最近文件和会话恢复继续保留，并扩展为通用文档打开入口。

## Key Changes

- 构建与依赖
  - 顶层 `CMakeLists.txt` 增加 `PYRAQT_ENABLE_OCCT` 选项，默认 `ON`。
  - 新增 `cmake/FindOCCT.cmake`，查找 OCCT 头文件和 STEP/BREP 最小库集，组装 `OCCT::OCCT` 导入目标。
  - 检测成功时设置 `PYRAQT_HAS_OCCT=1`，把模型导入源码加入 `pyraqt_core`；检测失败时保留可编译主线并输出清晰警告。
- 核心模型子系统
  - 新增 `src/core/cad` 子系统，提供 `CadFormat`、`CadImportSummary`、`CadImportManager`。
  - `.stp/.step` 使用 `STEPControl_Reader` 导入，`.brep` 使用 `BRepTools::Read` 导入。
  - 导入后统一统计根 shape 数量和基础拓扑数量，不解析颜色、名称树、装配层级与 PMI。
- UI 与文件打开流
  - 将当前中心工作区泛化为文档工作区，保留脚本标签能力，并新增只读 `CadSummaryWidget` 标签类型。
  - 打开入口统一按扩展名路由：`.py` 走脚本编辑链路，`.stp/.step/.brep` 走模型导入链路。
  - `File` 菜单中的 `Open Script` 调整为 `Open File`，过滤器覆盖 Python 和模型文件。
  - 文件树双击、最近文件和会话恢复共用同一条 `openPath()` 路由。
- 文档与说明
  - `README.md` 和 `docs/ARCHITECTURE.md` 补充 OCCT 是可选预装依赖，以及当前仅支持摘要导入展示。

## Public Interfaces

- `core::CadFormat`
- `core::CadImportSummary`
- `core::CadImportManager::isSupportedFile(path)`
- `core::CadImportManager::importFile(path)`
- 文档工作区新增统一入口：`openPath(path)`
- `ui::CadSummaryWidget::setSummary(const core::CadImportSummary &summary)`

## Test Plan

- 无 OCCT 环境下配置和构建继续通过，模型功能禁用。
- 有 OCCT 环境下配置和构建通过，`PYRAQT_HAS_OCCT=1` 生效。
- 扩展名识别覆盖 `.py`、`.stp`、`.step`、`.brep` 与未知扩展。
- 会话恢复包含模型路径时仍保持去重与缺失文件过滤。
- 有 OCCT 时增加最小 `.brep` 与 `.step` 导入测试，断言导入成功且基础拓扑计数有效。

## Assumptions

- 本阶段不做 3D 视口、选择、高亮、测量、剖切、网格化导出和属性树。
- `.step` 与 `.stp` 视为同一格式。
- 模型标签为只读摘要页，不支持保存或编辑。
- 不修改现有 CI 安装依赖策略；OCCT 相关测试只在检测到 OCCT 的环境中编译和运行。
