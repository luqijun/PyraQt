# Contributing

## 开发原则

- 维持 `core` 不依赖 `ui`
- 不修改 `python-from-qgis-project/`
- 默认保持 Qt5/Qt6 兼容
- 优先让无网络环境也能完成本地构建

## 提交流程

- 使用 Conventional Commits
- 修改 C++ 文件后运行本地构建与测试
- 新增用户可见文案时同步更新翻译

## 建议命令

```bash
cmake --preset linux-debug-qt-auto
cmake --build --preset linux-debug-qt-auto
ctest --preset linux-debug-qt-auto
```
