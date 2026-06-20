# PyraQt Phase 7 开发计划：OCCT 模型视图与属性面板首版

## Summary

- 本阶段把当前“模型摘要页”升级为真正的 OCCT 3D 模型视图：主窗口中央标签页直接显示 `.stp/.step/.brep` 模型，右侧全局 `Properties` dock 显示当前模型或当前选中子形的属性。
- 交付范围锁定为“平衡版浏览器”：支持旋转、平移、缩放、Fit All、标准视角、线框/着色/带边着色、边/面/实体选择切换、hover 高亮、单选属性查看；不做隐藏/隔离/多选/编辑。
- 属性面板采用共享全局 dock 方案，脚本页保持原有行为，模型页升级为交互式 3D 视图。

## Key Changes

- 构建与依赖
  - 扩展 OCCT 检测要求，覆盖 `AIS`、`V3d`、`Aspect`、`Graphic3d`、`OpenGl`、`SelectMgr` 及 STEP/data-exchange 开发头。
  - Qt 侧补齐模型视口依赖，兼容 Qt5 与 Qt6。
  - 无完整 OCCT 可视化环境时主线仍可配置，模型视图功能禁用并明确提示。
- 核心模型与属性
  - 新增 `ModelDocument`、`ModelSelectionInfo`、`ModelDisplayMode`、`ModelSelectionMode`。
  - `ModelImportManager` 从只返回摘要升级为返回模型文档与摘要，补齐包围盒和几何量统计。
  - 新增属性提取服务，将模型整体属性和选中子形属性整理为 UI 可显示项。
- 3D 视口与交互
  - 新增 `OcctModelViewWidget` 和模型文档页，承载真实模型显示。
  - 首版支持旋转、平移、缩放、Fit All、标准视角、线框/着色/带边着色、形体/面/边/点选择、hover 高亮、单选。
- 主窗口与工作区
  - 右侧 `Properties` dock 替换为真实属性面板。
  - 模型文件在工作区中打开为 3D 视图标签页，不再只显示摘要。
  - 主窗口新增模型动作并注册到 Command Palette。

## Public Interfaces

- `core::ModelDocument`
- `core::ModelSelectionInfo`
- `core::ModelDisplayMode`
- `core::ModelSelectionMode`
- `core::ModelImportManager::importFile(path)` 返回模型文档结果
- `ui::OcctModelViewWidget`
- `ui::ModelPropertiesPanel`
- `ui::EditorWorkspaceWidget` 继续暴露 `openPath(path)`，模型分支改为视图文档页

## Test Plan

- 有完整 OCCT 可视化与 data-exchange 开发环境时，配置与编译通过。
- 缺少 OCCT 或缺少 STEP/data-exchange 头时，配置阶段给出清晰禁用原因。
- 模型格式识别、属性提取、显示模式与选择模式状态机有单元测试。
- UI 冒烟覆盖模型打开、视角切换、显示模式切换、选择联动属性面板、会话恢复。

## Assumptions

- 本阶段不做 CAD 编辑、隐藏/隔离、多选、剖切、测量、导出、装配树或特征树。
- 选择交互首版以单选优先，属性窗口只展示一个当前对象。
- STEP 名称、颜色、装配层级等高级元数据不在本阶段承诺范围内。
