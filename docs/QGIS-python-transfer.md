# QGIS Python嵌入架构迁移至自定义Qt C++项目实施方案
QGIS 的 Python 嵌入本质是一套**「Qt主进程托管CPython解释器 + 绑定层桥接 + 统一执行调度器 + 线程安全管控」**的成熟框架。迁移时不需要全盘照搬老旧的SIP编译体系（QGIS因历史绑定PyQt才使用SIP），可以分为两条迁移路线：

> ✅ **路线1（新项目首选）：Qt C++ + CPython + PyBind11**（现代方案，开发效率高、编译简单，推荐）
> ⚠️ **路线2（高度复刻QGIS原生架构）：Qt C++ + CPython + SIP**（和QGIS架构1:1对齐，但编译繁琐、配置复杂，仅需要深度融合PyQt时使用）

下面先做**QGIS架构 ↔ 自定义Qt项目模块映射**，再分步落地迁移、给出代码框架、线程模型、内存管理、初始化时序，最后说明坑点与优化方案。

## 一、架构模块映射对照表（核心迁移依据）
把QGIS源码中的Python相关组件，一一映射到你的Qt项目中：

| QGIS核心类/模块 | 作用 | 自定义Qt项目中需要新建的类 |
|----|----|----|
| `QgsPythonUtils` | CPython解释器初始化、环境配置、路径管理、全局变量注入 | `PythonEnvManager`（解释器全局管理器） |
| `QgsPythonRunner` | Python代码统一执行入口、GIL锁封装、代码执行、异常捕获 | `PythonRunner`（脚本执行器） |
| SIP绑定目录 | C++类导出为Python模块、双向类型转换 | PyBind11扩展库（推荐）/ SIP绑定工程 |
| `QgsPythonConsole` | 交互式控制台、stdout/stderr重定向、命令交互 | `PythonConsoleWidget`（Qt控制台控件） |
| `QgsPluginManager` | 扫描并加载Python插件 | `PythonPluginLoader`（插件加载器） |
| `QgsTask` | 后台异步任务、隔离Python耗时运算、防止阻塞GUI | 封装`PythonTask`继承`QRunnable/QThreadPool` |
| `iface`全局对象 | Python侧访问Qt主窗口、内核接口 | 注入自定义`api`全局对象 |
| Processing框架 | Python算法脚本调度 | 自定义脚本任务调度模块 |

迁移后的整体架构分层（对标QGIS五层架构）：
```
┌─────────────────────────────┐
│ 脚本层：用户脚本/插件/交互式控制台
├─────────────────────────────┤
│ 调度层：PythonRunner + PluginLoader（统一执行入口）
├─────────────────────────────┤
│ 绑定层：PyBind11扩展模块（mycore.so/pyd）C++↔Python桥接
├─────────────────────────────┤
│ 解释器层：嵌入式CPython3虚拟机（全局单例）
├─────────────────────────────┤
│ 内核层：Qt C++主程序、MainWindow、业务内核、GUI事件循环
└─────────────────────────────┘
```

## 二、迁移整体实施步骤
### 步骤1：确定解释器与绑定器选型
#### 方案A：PyBind11方案（推荐，90%新项目使用）
1. 嵌入**独立便携版CPython3**（不要依赖系统Python，和QGIS打包内置Python思路一致）；
2. 使用PyBind11将自己的C++类导出为Python扩展模块；
3. 自行封装`PythonEnvManager`、`PythonRunner`复刻QGIS的执行调度逻辑。

**优点**：
- 编译配置简单，CMake原生支持；
- 语法现代，支持C++17/20，类型转换自动处理；
- 社区活跃，文档丰富，调试方便；
- 可以自由控制C++对象的内存所有权，规避QGIS常见的GC野指针问题。

**缺点**：和PyQt信号槽绑定不如SIP原生顺滑。

#### 方案B：SIP完整复刻QGIS架构（仅特殊场景使用）
完全照搬QGIS的SIP编译流程：编写`.sip`接口描述文件 → sip-tools生成包装代码 → 编译成Python扩展模块。
**缺点**：
- 配置极度繁琐，Qt版本、SIP版本、PyQt版本必须严格匹配；
- 编译链路长，调试困难；
- SIP语法老旧，学习成本高；
> 除非你的项目重度依赖PyQt交互、需要和QGIS二次开发生态打通，否则不建议复刻SIP架构。

下文以**推荐的PyBind11路线**为主进行迁移讲解。

---

### 步骤2：复刻 QgsPythonUtils → 编写 PythonEnvManager 解释器管理器
对应QGIS中`initPython()`整套初始化逻辑，**初始化顺序必须严格遵守：先创建QApplication，再初始化Python解释器（QGIS也是这个顺序，颠倒会导致Qt事件循环卡死）**。

管理器需要实现的功能：
1. 调用CPython原生API `Py_Initialize()` 创建虚拟机；
2. 配置环境变量：`PYTHONHOME`、`PYTHONPATH`，隔离系统Python；
3. 修改`sys.path`，注入插件目录、自定义扩展模块路径；
4. 保存主线程的`PyThreadState`线程状态；
5. 向Python全局命名空间注入C++指针（对标QGIS的`iface`）；
6. 重定向`stdout/stderr`，捕获print输出到Qt界面；
7. 程序退出时安全销毁解释器`Py_Finalize()`。

#### 初始化时序（严格对齐QGIS启动流程）
```
1. main() → 创建 QApplication a(argc, argv)
2. 创建主窗口 MainWindow w
3. 实例化 PythonEnvManager::instance()
4. 调用 envManager->initialize() 初始化CPython
5. 加载自定义PyBind11扩展模块
6. 注入全局对象 api（替代QGIS的iface）
7. 加载Python插件目录
8. 显示窗口，进入Qt事件循环 exec()
```

---

### 步骤3：复刻 QgsPythonRunner → 封装统一执行器
`QgsPythonRunner`是QGIS整个Python架构的核心，作用是**统一收口所有Python代码执行、自动管理GIL锁、捕获异常、隔离调用逻辑**，迁移时必须原样复刻三层执行接口：

1. **runCode(QString code)**：执行多行代码，对应Python `exec()`（插件、大段脚本）
2. **runEval(QString expr)**：执行表达式并返回结果，对应Python `eval()`
3. **runCommand(QString cmd)**：单行命令执行，供控制台使用

#### 核心要点：GIL锁自动封装
CPython同一时刻只有一个线程持有GIL全局解释器锁，QGIS在Runner内部做了封装：
- 执行Python代码前：`PyGILState_STATE gil = PyGILState_Ensure();`
- 执行完毕后：`PyGILState_Release(gil);`

你需要把GIL加解锁封装在Runner内部，上层业务代码不需要手动管理锁，和QGIS设计保持一致。

Runner内部调用栈：
```
上层调用 → PythonRunner::runCode()
    → 上锁GIL
    → PyRun_String(code, Py_file_input, globalDict, localDict)
    → 捕获Python异常、提取报错信息
    → 释放GIL
    → 返回执行结果与异常日志
```

---

### 步骤4：搭建PyBind11绑定层，实现C++与Python双向调用
对应QGIS的SIP绑定层，实现双向互通：
#### 方向1：Python调用C++内核（类似PyQGIS调用qgis.core）
使用PyBind11导出：
- 业务C++类、成员函数、属性；
- Qt信号槽（PyBind11支持绑定Qt信号）；
- 内核功能接口（图层管理、计算函数、窗口控制等）。

#### 方向2：C++主动调用Python函数（类似QGIS回调宏、表达式函数）
通过Runner查找Python全局函数对象，调用并传参，支持：
- 调用Python自定义回调函数；
- 执行项目启动钩子、事件回调；
- 获取Python函数返回值回传给C++。

> 内存所有权设计（重点，QGIS高频崩溃根源）：
> QGIS的SIP需要手动设置`Transfer ownership`标记来控制GC是否释放C++对象；
> PyBind11可以通过`py::return_value_policy`策略统一管理，避免Python垃圾回收误删除C++常驻对象。

---

### 步骤5：复刻Python交互式控制台
对标QGIS的`QgsPythonConsole`控件，基于`QPlainTextEdit`实现：
1. 重定向Python的`sys.stdout`、`sys.stderr`，将print输出、报错信息转发到Qt文本框；
2. 维护独立的全局/局部命名空间，命令之间变量持久化；
3. 实现命令历史、回车执行、语法高亮；
4. 控制台输入的命令全部交给`PythonRunner`执行。

---

### 步骤6：复刻插件加载系统（对标QgsPluginManager）
设计插件规范，模仿QGIS插件机制：
1. 定义插件目录：`./python/plugins/`；
2. 约定插件入口文件`__init__.py` + 配置文件`metadata.json`（对应QGIS的metadata.txt）；
3. 启动时遍历目录，动态import插件模块；
4. 实例化插件类，并将全局`api`对象（等价iface）传入插件；
5. 提供插件启用/禁用开关。

---

### 步骤7：复刻QgsTask异步任务线程模型
QGIS为了解决GIL阻塞GUI的问题，设计了`QgsTask`后台任务框架，迁移规则：
1. **Qt主线程严禁执行长时间Python代码**；
2. 耗时脚本放到子线程`QThreadPool`中运行；
3. 子线程内执行Python代码时临时获取GIL，执行完立刻释放；
4. Python子线程**禁止直接操作Qt控件**，必须通过`Qt::QueuedConnection`信号槽回调到主线程刷新界面。

## 三、可直接编译运行的Qt+PyBind11代码框架示例
项目结构：
```
MyQtApp/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp
│  ├─ mainwindow.h/cpp
│  ├─ python/
│  │  ├─ python_env_manager.h/cpp  // 对标QgsPythonUtils
│  │  └─ python_runner.h/cpp       // 对标QgsPythonRunner
├─ bindings/
│  └─ mycore.cpp                   // PyBind11绑定代码
└─ python/                         // 插件、脚本目录
```

### 1. PythonEnvManager 头文件（精简版）
```cpp
#ifndef PYTHONENVMANAGER_H
#define PYTHONENVMANAGER_H

#include <QString>
#include <Python.h>

class PythonEnvManager
{
public:
    static PythonEnvManager* instance();
    bool initialize();
    void finalize();

    PyObject* globalDict() const;

private:
    PythonEnvManager();
    ~PythonEnvManager();
    bool redirectStdIO();   // 重定向输出
    void injectGlobalApi(); // 注入api全局对象（替代iface）

    PyObject* m_globalDict = nullptr;
    PyThreadState* m_mainThreadState = nullptr;
};

#endif
```

### 2. PythonRunner 执行器核心逻辑（GIL自动封装）
```cpp
#include "python_runner.h"
#include "python_env_manager.h"
#include <Python.h>
#include <QDebug>

QVariant PythonRunner::runCode(const QString &code)
{
    PyGILState_STATE gil = PyGILState_Ensure();

    PyObject* res = PyRun_String(
        code.toStdString().c_str(),
        Py_file_input,
        PythonEnvManager::instance()->globalDict(),
        PythonEnvManager::instance()->globalDict()
    );

    QVariant result;
    if (res)
    {
        result = pyObjToVariant(res);
        Py_DECREF(res);
    }
    else
    {
        // 提取Python异常信息
        PyErr_Print();
    }

    PyGILState_Release(gil);
    return result;
}
```

### 3. PyBind11绑定示例 bindings/mycore.cpp
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/qt/qvariant.h>
#include "mainwindow.h"

namespace py = pybind11;

PYBIND11_MODULE(mycore, m)
{
    m.doc() = "Custom Qt C++ Core Module";

    // 导出C++内核类给Python调用
    py::class_<MainWindow>(m, "MainWindow")
        .def("getTitle", &MainWindow::windowTitle)
        .def("setText", &MainWindow::setTextEdit);
}
```

### 4. main.cpp 启动顺序
```cpp
#include <QApplication>
#include "mainwindow.h"
#include "python/python_env_manager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. 先创建Qt应用，再初始化Python（关键顺序）
    PythonEnvManager::instance()->initialize();

    MainWindow w;
    w.show();

    return a.exec();
}
```

## 四、迁移过程中必须注意的QGIS架构坑点
### 1. 初始化顺序禁忌
❌ 错误：先`Py_Initialize()`，再创建`QApplication`
✅ 正确：**QApplication必须在CPython初始化之前创建**（QGIS源码严格遵守此顺序，颠倒会造成Qt事件循环紊乱、界面无响应）

### 2. GIL锁使用规范
- 只有Runner统一管理GIL，禁止在任意C++函数随意加锁解锁；
- 子线程每次执行Python代码都要重新获取GIL，不能跨线程持有；
- 长时间运算要频繁释放GIL，避免Qt界面卡死。

### 3. C++对象内存生命周期（QGIS崩溃高发点）
- 常驻内核对象（主窗口、管理器）设置为**Python不可回收**；
- 临时创建的对象通过PyBind11策略控制所有权，防止Python GC释放C++指针；
- 不要把Qt父控件交给Python持有，避免双重析构。

### 4. Python环境路径隔离
QGIS内置独立Python解释器，自定义项目也要打包**便携版Python**，不要链接系统Python，防止：
- 版本冲突、库路径混乱；
- 打包发布后用户环境缺失依赖。

### 5. 双向调用死锁问题
- Python调用C++耗时函数 → 不要阻塞GIL；
- C++回调Python函数时，检查当前线程是否持有GIL，避免嵌套上锁死锁。

## 五、SIP架构完整复刻方案（简要配置思路）
如果需要1:1复刻QGIS原生SIP架构，步骤如下：
1. 安装sip-tools、PyQt对应版本；
2. 仿照QGIS `/sip` 目录编写`.sip`接口描述文件，定义C++类导出规则、所有权标记；
3. 编写sip编译脚本，生成包装cpp代码；
4. CMake编译生成`.pyd`/`.so`扩展模块；
5. 沿用QGIS的`QgsPythonRunner`逻辑，整体架构和原版完全一致。

## 六、整体总结迁移思路
1. **架构思路照搬QGIS**：单进程内嵌CPython + 统一执行器 + 全局解释器管理器 + 线程调度模型；
2. **技术栈择优改造**：抛弃老旧SIP，改用PyBind11降低开发成本；
3. **模块一一映射重构**：
   `QgsPythonUtils→PythonEnvManager`、`QgsPythonRunner→PythonRunner`、`插件管理器→PythonPluginLoader`；
4. **对齐初始化时序、GIL管理、内存所有权三大核心规则**；
5. 最后实现控制台、插件系统、异步任务调度，完成整套迁移。

如果你需要，我可以：
1. 输出完整可编译的CMake工程模板；
2. 给出stdout/stderr重定向完整代码；
3. 画出架构UML类图；
4. 补充C++调用Python函数的完整示例代码。