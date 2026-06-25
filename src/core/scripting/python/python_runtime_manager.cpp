#include "core/scripting/python/python_runtime_manager.h"

#include "core/config/config_manager.h"

#pragma push_macro("slots")
#undef slots
#include <Python.h>
#pragma pop_macro("slots")

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace pyraqt::core {

namespace {

PythonRuntimeManager *s_runtimeManager = nullptr;

QString dataRootPath()
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QStringLiteral("PYRAQT_DATA_DIR"))) {
        return environment.value(QStringLiteral("PYRAQT_DATA_DIR"));
    }
    return QDir::temp().filePath(QStringLiteral("PyraQt"));
}

bool writeTextFile(const QString &path, const QString &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(content.toUtf8());
    return true;
}

PyObject *returnNone()
{
    Py_RETURN_NONE;
}

QString pyUnicodeArg(PyObject *args, int index)
{
    PyObject *item = PyTuple_GetItem(args, index);
    if (item == nullptr) {
        return {};
    }
    PyObject *stringObject = PyObject_Str(item);
    if (stringObject == nullptr) {
        return {};
    }
    const QString value = QString::fromUtf8(PyUnicode_AsUTF8(stringObject));
    Py_DECREF(stringObject);
    return value;
}

QStringList pyListArg(PyObject *args, int index)
{
    QStringList values;
    PyObject *item = PyTuple_GetItem(args, index);
    if (item == nullptr || !PySequence_Check(item)) {
        return values;
    }
    const Py_ssize_t size = PySequence_Size(item);
    for (Py_ssize_t row = 0; row < size; ++row) {
        PyObject *entry = PySequence_GetItem(item, row);
        if (entry == nullptr) {
            continue;
        }
        PyObject *stringObject = PyObject_Str(entry);
        if (stringObject != nullptr) {
            values.push_back(QString::fromUtf8(PyUnicode_AsUTF8(stringObject)));
            Py_DECREF(stringObject);
        }
        Py_DECREF(entry);
    }
    return values;
}

QVariantMap pyDictArg(PyObject *args, int index)
{
    QVariantMap values;
    PyObject *item = PyTuple_GetItem(args, index);
    if (item == nullptr || !PyDict_Check(item)) {
        return values;
    }

    PyObject *key = nullptr;
    PyObject *value = nullptr;
    Py_ssize_t position = 0;
    while (PyDict_Next(item, &position, &key, &value)) {
        PyObject *keyString = PyObject_Str(key);
        PyObject *valueString = PyObject_Str(value);
        if (keyString != nullptr && valueString != nullptr) {
            values.insert(QString::fromUtf8(PyUnicode_AsUTF8(keyString)), QString::fromUtf8(PyUnicode_AsUTF8(valueString)));
        }
        Py_XDECREF(keyString);
        Py_XDECREF(valueString);
    }
    return values;
}

} // namespace

PyObject *pyraqtNativeEmitStdout(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitStdoutFromPython(pyUnicodeArg(args, 0));
    }
    return returnNone();
}

PyObject *pyraqtNativeEmitStderr(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitStderrFromPython(pyUnicodeArg(args, 0));
    }
    return returnNone();
}

PyObject *pyraqtNativeSetStatus(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitStatusFromPython(pyUnicodeArg(args, 0));
    }
    return returnNone();
}

PyObject *pyraqtNativeShowMessage(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitDialogFromPython(pyUnicodeArg(args, 0));
    }
    return returnNone();
}

PyObject *pyraqtNativeLog(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitLogFromPython(pyUnicodeArg(args, 0), pyUnicodeArg(args, 1));
    }
    return returnNone();
}

PyObject *pyraqtNativeRegisterCommand(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitCommandRegistrationFromPython(
            pyUnicodeArg(args, 0),
            pyUnicodeArg(args, 1),
            pyUnicodeArg(args, 2),
            pyListArg(args, 3));
    }
    return returnNone();
}

PyObject *pyraqtNativeCanAccessFileSystem(PyObject *, PyObject *args)
{
    if (s_runtimeManager == nullptr) {
        Py_RETURN_FALSE;
    }
    if (s_runtimeManager->isFileSystemAccessAllowedFromPython(pyUnicodeArg(args, 0), pyUnicodeArg(args, 1))) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyObject *pyraqtNativeLoadMacro(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitMacroLoadFromPython(pyUnicodeArg(args, 0));
    }
    return returnNone();
}

PyObject *pyraqtNativeTriggerMacro(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitMacroTriggerFromPython(pyUnicodeArg(args, 0));
    }
    return returnNone();
}

PyObject *pyraqtNativeRegisterExpression(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitExpressionRegistrationFromPython(pyUnicodeArg(args, 0), pyUnicodeArg(args, 1));
    }
    return returnNone();
}

PyObject *pyraqtNativeEvaluateExpression(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitExpressionEvaluationFromPython(pyUnicodeArg(args, 0), pyListArg(args, 1));
    }
    return returnNone();
}

PyObject *pyraqtNativeRegisterProcessing(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitProcessingRegistrationFromPython(pyUnicodeArg(args, 0), pyUnicodeArg(args, 1));
    }
    return returnNone();
}

PyObject *pyraqtNativeRunProcessing(PyObject *, PyObject *args)
{
    if (s_runtimeManager != nullptr) {
        s_runtimeManager->emitProcessingRunFromPython(pyUnicodeArg(args, 0), pyDictArg(args, 1));
    }
    return returnNone();
}

namespace {

PyMethodDef pyraqtNativeMethods[] = {
    {"emit_stdout", pyraqtNativeEmitStdout, METH_VARARGS, "Emit stdout text to PyraQt."},
    {"emit_stderr", pyraqtNativeEmitStderr, METH_VARARGS, "Emit stderr text to PyraQt."},
    {"set_status", pyraqtNativeSetStatus, METH_VARARGS, "Request a status bar message."},
    {"show_message", pyraqtNativeShowMessage, METH_VARARGS, "Request an application message."},
    {"log", pyraqtNativeLog, METH_VARARGS, "Emit a log message."},
    {"register_command", pyraqtNativeRegisterCommand, METH_VARARGS, "Register a Python-backed command."},
    {"can_access_filesystem", pyraqtNativeCanAccessFileSystem, METH_VARARGS, "Check whether Python file helpers may access a path."},
    {"load_macro", pyraqtNativeLoadMacro, METH_VARARGS, "Load project macro code."},
    {"trigger_macro", pyraqtNativeTriggerMacro, METH_VARARGS, "Trigger a project macro."},
    {"register_expression", pyraqtNativeRegisterExpression, METH_VARARGS, "Register an expression function."},
    {"evaluate_expression", pyraqtNativeEvaluateExpression, METH_VARARGS, "Evaluate an expression function."},
    {"register_processing", pyraqtNativeRegisterProcessing, METH_VARARGS, "Register a processing algorithm."},
    {"run_processing", pyraqtNativeRunProcessing, METH_VARARGS, "Run a processing algorithm."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef pyraqtNativeModule = {
    PyModuleDef_HEAD_INIT,
    "pyraqt_native",
    "Native bridge for embedded PyraQt Python.",
    -1,
    pyraqtNativeMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

PyObject *createPyraqtNativeModule()
{
    return PyModule_Create(&pyraqtNativeModule);
}

} // namespace

PythonRuntimeManager::PythonRuntimeManager(ConfigManager &configManager, QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
{
}

PythonRuntimeManager::~PythonRuntimeManager()
{
    shutdownEmbedded();
}

QString PythonRuntimeManager::interpreterPath() const
{
    return m_configManager.value(QStringLiteral("python.interpreter_path"), QStringLiteral("python3")).toString();
}

void PythonRuntimeManager::setInterpreterPath(const QString &path)
{
    m_configManager.setValue(QStringLiteral("python.interpreter_path"), path);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit interpreterChanged(path);
}

void PythonRuntimeManager::setExecutionTimeoutMs(int timeoutMs)
{
    m_configManager.setValue(QStringLiteral("python.execution_timeout_ms"), timeoutMs);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit executionTimeoutChanged(timeoutMs);
}

bool PythonRuntimeManager::isInterpreterAvailable() const
{
    const QString path = interpreterPath();
    if (QFileInfo::exists(path) && QFileInfo(path).isExecutable()) {
        return true;
    }
    return !QStandardPaths::findExecutable(path).isEmpty();
}

QString PythonRuntimeManager::pythonVersion() const
{
    QProcess process;
    process.start(interpreterPath(), {QStringLiteral("--version")});
    if (!process.waitForFinished(3000)) {
        return QStringLiteral("Unavailable");
    }

    const QString stdoutOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stderrOutput = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QString version = !stdoutOutput.isEmpty() ? stdoutOutput : stderrOutput;
    return version.isEmpty() ? QStringLiteral("Unavailable") : version;
}

int PythonRuntimeManager::executionTimeoutMs() const
{
    return m_configManager.value(QStringLiteral("python.execution_timeout_ms"), 15000).toInt();
}

bool PythonRuntimeManager::isEnabled() const
{
    return m_configManager.value(QStringLiteral("python.enabled"), true).toBool();
}

bool PythonRuntimeManager::isEmbeddedInitialized() const
{
    return m_embeddedInitialized;
}

bool PythonRuntimeManager::macrosEnabled() const
{
    return m_configManager.value(QStringLiteral("python.macros_enabled"), false).toBool();
}

void PythonRuntimeManager::setMacrosEnabled(bool enabled)
{
    m_configManager.setValue(QStringLiteral("python.macros_enabled"), enabled);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
}

bool PythonRuntimeManager::fileSystemAccessEnabled() const
{
    return m_configManager.value(QStringLiteral("python.filesystem_access_enabled"), true).toBool();
}

void PythonRuntimeManager::setFileSystemAccessEnabled(bool enabled)
{
    m_configManager.setValue(QStringLiteral("python.filesystem_access_enabled"), enabled);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
}

bool PythonRuntimeManager::useIsolatedExecutionByDefault() const
{
    return m_configManager.value(QStringLiteral("python.use_isolated_execution_by_default"), false).toBool();
}

void PythonRuntimeManager::setUseIsolatedExecutionByDefault(bool enabled)
{
    m_configManager.setValue(QStringLiteral("python.use_isolated_execution_by_default"), enabled);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
}

int PythonRuntimeManager::consoleHistoryLimit() const
{
    return m_configManager.value(QStringLiteral("python.console_history_limit"), 200).toInt();
}

void PythonRuntimeManager::setConsoleHistoryLimit(int limit)
{
    m_configManager.setValue(QStringLiteral("python.console_history_limit"), limit);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
}

bool PythonRuntimeManager::codeCompletionEnabled() const
{
    return m_configManager.value(QStringLiteral("python.code_completion_enabled"), true).toBool();
}

void PythonRuntimeManager::setCodeCompletionEnabled(bool enabled)
{
    m_configManager.setValue(QStringLiteral("python.code_completion_enabled"), enabled);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
}

int PythonRuntimeManager::completionTriggerThreshold() const
{
    return m_configManager.value(QStringLiteral("python.completion_trigger_threshold"), 2).toInt();
}

void PythonRuntimeManager::setCompletionTriggerThreshold(int threshold)
{
    m_configManager.setValue(QStringLiteral("python.completion_trigger_threshold"), qMax(1, threshold));
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
}

QStringList PythonRuntimeManager::pluginPaths() const
{
    QStringList paths = defaultPluginPaths();
    const QStringList configured = m_configManager.value(QStringLiteral("python.plugin_paths")).toStringList();
    for (const QString &path : configured) {
        if (!path.isEmpty() && !paths.contains(path)) {
            paths.push_back(path);
        }
    }
    return paths;
}

QString PythonRuntimeManager::pythonPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("python"));
}

PyObject *PythonRuntimeManager::globalDict() const
{
    return m_globalDict;
}

bool PythonRuntimeManager::initializeEmbedded(const QStringList &extraPythonPaths, const QStringList &extraPluginPaths)
{
    if (!isEnabled()) {
        return false;
    }
    if (m_embeddedInitialized) {
        return true;
    }

    s_runtimeManager = this;
    if (!appendPyraNativeModule()) {
        return false;
    }

    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    PyGILState_STATE gil = PyGILState_Ensure();
    m_mainModule = PyImport_AddModule("__main__");
    if (m_mainModule == nullptr) {
        PyGILState_Release(gil);
        return false;
    }
    m_globalDict = PyModule_GetDict(m_mainModule);
    Py_XINCREF(m_mainModule);
    Py_XINCREF(m_globalDict);

    const bool pathConfigured = configureSysPath(extraPythonPaths, extraPluginPaths);
    if (pathConfigured) {
        m_embeddedInitialized = true;
    }
    const bool configured = pathConfigured && installPyraPackage(extraPythonPaths, extraPluginPaths);
    if (configured) {
        emit embeddedStateChanged(true);
    } else {
        m_embeddedInitialized = false;
    }

    PyGILState_Release(gil);
    return configured;
}

void PythonRuntimeManager::shutdownEmbedded()
{
    if (!m_embeddedInitialized) {
        return;
    }

    PyGILState_STATE gil = PyGILState_Ensure();
    Py_XDECREF(m_globalDict);
    Py_XDECREF(m_mainModule);
    m_globalDict = nullptr;
    m_mainModule = nullptr;
    m_mainThreadState = nullptr;
    m_embeddedInitialized = false;
    PyGILState_Release(gil);
    s_runtimeManager = nullptr;
    emit embeddedStateChanged(false);
}

ScriptResult PythonRuntimeManager::runString(const QString &code, bool singleInput)
{
    ScriptResult result;
    if (!initializeEmbedded()) {
        result.traceback = QStringLiteral("Embedded Python is not initialized.");
        result.exitCode = 1;
        return result;
    }

    QElapsedTimer timer;
    timer.start();
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *object = PyRun_String(code.toUtf8().constData(), singleInput ? Py_single_input : Py_file_input, m_globalDict, m_globalDict);
    if (object == nullptr || PyErr_Occurred() != nullptr) {
        result.success = false;
        result.exitCode = 1;
        result.traceback = getTraceback();
    } else {
        result.success = true;
        result.resultText = pyObjectToString(object);
    }
    Py_XDECREF(object);
    PyGILState_Release(gil);
    result.durationMs = timer.elapsed();
    return result;
}

ScriptResult PythonRuntimeManager::runFile(const QString &filePath, const QStringList &arguments)
{
    ScriptResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.traceback = QStringLiteral("Cannot open file: %1").arg(filePath);
        result.exitCode = 1;
        return result;
    }
    QStringList argv = arguments;
    argv.prepend(filePath);
    const bool argvSet = setArgv(argv);
    Q_UNUSED(argvSet)
    result = runString(QString::fromUtf8(file.readAll()), false);
    return result;
}

ScriptResult PythonRuntimeManager::evalString(const QString &expression)
{
    ScriptResult result;
    if (!initializeEmbedded()) {
        result.traceback = QStringLiteral("Embedded Python is not initialized.");
        result.exitCode = 1;
        return result;
    }

    QElapsedTimer timer;
    timer.start();
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *object = PyRun_String(expression.toUtf8().constData(), Py_eval_input, m_globalDict, m_globalDict);
    if (object == nullptr || PyErr_Occurred() != nullptr) {
        result.success = false;
        result.exitCode = 1;
        result.traceback = getTraceback();
    } else {
        result.success = true;
        result.resultText = pyObjectToString(object);
    }
    Py_XDECREF(object);
    PyGILState_Release(gil);
    result.durationMs = timer.elapsed();
    return result;
}

QStringList PythonRuntimeManager::completeMembers(const QString &objectExpression, const QString &contextCode)
{
    QStringList members;
    if (objectExpression.trimmed().isEmpty() || !initializeEmbedded()) {
        return members;
    }

    QString contextLiteral = contextCode;
    contextLiteral.replace('\\', QStringLiteral("\\\\"));
    contextLiteral.replace('\"', QStringLiteral("\\\""));
    contextLiteral.replace('\n', QStringLiteral("\\n"));
    contextLiteral.replace('\r', QStringLiteral("\\r"));

    QString objectLiteral = objectExpression;
    objectLiteral.replace('\\', QStringLiteral("\\\\"));
    objectLiteral.replace('\"', QStringLiteral("\\\""));

    const QString helper = QStringLiteral(
        "_pyraqt_completion_locals = {}\n"
        "exec(\"%1\", globals(), _pyraqt_completion_locals)\n"
        "_pyraqt_completion_target = eval(\"%2\", globals(), _pyraqt_completion_locals)\n")
                               .arg(contextLiteral, objectLiteral);
    const ScriptResult setup = runString(helper, false);
    if (!setup.success) {
        return members;
    }

    const ScriptResult result = evalString(QStringLiteral("sorted([name for name in dir(_pyraqt_completion_target) if isinstance(name, str)])"));
    if (!result.success || result.resultText.isEmpty()) {
        const ScriptResult cleanupResult = runString(
            QStringLiteral("globals().pop('_pyraqt_completion_locals', None)\nglobals().pop('_pyraqt_completion_target', None)"),
            false);
        Q_UNUSED(cleanupResult)
        return members;
    }

    QString text = result.resultText;
    text.remove(QLatin1Char('['));
    text.remove(QLatin1Char(']'));
    text.remove(QLatin1Char('\''));
    for (const QString &member : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString trimmed = member.trimmed();
        if (!trimmed.isEmpty()) {
            members.push_back(trimmed);
        }
    }
    members.removeDuplicates();
    members.sort(Qt::CaseInsensitive);
    const ScriptResult cleanupResult = runString(
        QStringLiteral("globals().pop('_pyraqt_completion_locals', None)\nglobals().pop('_pyraqt_completion_target', None)"),
        false);
    Q_UNUSED(cleanupResult)
    return members;
}

bool PythonRuntimeManager::setArgv(const QStringList &arguments)
{
    if (!initializeEmbedded()) {
        return false;
    }

    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *sysModule = PyImport_ImportModule("sys");
    PyObject *argsObject = PyList_New(arguments.size());
    if (sysModule == nullptr || argsObject == nullptr) {
        Py_XDECREF(sysModule);
        Py_XDECREF(argsObject);
        PyGILState_Release(gil);
        return false;
    }

    for (int index = 0; index < arguments.size(); ++index) {
        PyList_SET_ITEM(argsObject, index, PyUnicode_FromString(arguments.at(index).toUtf8().constData()));
    }
    const bool ok = PyObject_SetAttrString(sysModule, "argv", argsObject) == 0;
    Py_DECREF(argsObject);
    Py_DECREF(sysModule);
    PyGILState_Release(gil);
    return ok;
}

QString PythonRuntimeManager::getTraceback()
{
    QString result;
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);

    PyObject *tracebackModule = PyImport_ImportModule("traceback");
    if (tracebackModule != nullptr) {
        PyObject *formatException = PyObject_GetAttrString(tracebackModule, "format_exception");
        if (formatException != nullptr) {
            PyObject *list = PyObject_CallFunctionObjArgs(
                formatException,
                type ? type : Py_None,
                value ? value : Py_None,
                traceback ? traceback : Py_None,
                nullptr);
            if (list != nullptr) {
                PyObject *separator = PyUnicode_FromString("");
                PyObject *joined = PyUnicode_Join(separator, list);
                result = pyObjectToString(joined);
                Py_XDECREF(joined);
                Py_XDECREF(separator);
                Py_DECREF(list);
            }
            Py_DECREF(formatException);
        }
        Py_DECREF(tracebackModule);
    }

    if (result.isEmpty() && value != nullptr) {
        result = pyObjectToString(value);
    }

    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
    return result;
}

QString PythonRuntimeManager::getErrorText()
{
    if (!PyErr_Occurred()) {
        return {};
    }
    return getTraceback();
}

void PythonRuntimeManager::injectGlobalObject(const QString &name, PyObject *object)
{
    if (m_globalDict == nullptr || object == nullptr) {
        return;
    }
    PyDict_SetItemString(m_globalDict, name.toUtf8().constData(), object);
}

void PythonRuntimeManager::setRegistrationOwnerId(const QString &ownerId)
{
    m_registrationOwnerId = ownerId;
}

QString PythonRuntimeManager::registrationOwnerId() const
{
    return m_registrationOwnerId;
}

bool PythonRuntimeManager::appendPyraNativeModule()
{
    static bool appended = false;
    if (appended) {
        return true;
    }
    appended = PyImport_AppendInittab("pyraqt_native", &createPyraqtNativeModule) != -1;
    return appended;
}

bool PythonRuntimeManager::installPyraPackage(const QStringList &extraPythonPaths, const QStringList &extraPluginPaths)
{
    Q_UNUSED(extraPythonPaths)
    Q_UNUSED(extraPluginPaths)
    const QString rootPath = QDir(dataRootPath()).filePath(QStringLiteral("embedded_python"));
    const QString packagePath = QDir(rootPath).filePath(QStringLiteral("pyra"));
    QDir root(rootPath);
    if (!root.mkpath(QStringLiteral("pyra"))) {
        return false;
    }

    const QString module = QStringLiteral(
        "import pyraqt_native as _native\n"
        "\n"
        "class _Stream:\n"
        "    def __init__(self, sink):\n"
        "        self._sink = sink\n"
        "    def write(self, text):\n"
        "        if text:\n"
        "            self._sink(str(text))\n"
        "        return len(text) if text else 0\n"
        "    def flush(self):\n"
        "        return None\n"
        "\n"
        "class _App:\n"
        "    name = 'PyraQt'\n"
        "    version = '0.1.0'\n"
        "    @property\n"
        "    def interpreter_path(self):\n"
        "        import sys\n"
        "        return sys.executable\n"
        "\n"
        "class _Ui:\n"
        "    def show_message(self, text):\n"
        "        _native.show_message(str(text))\n"
        "    def set_status(self, text):\n"
        "        _native.set_status(str(text))\n"
        "\n"
        "class _Fs:\n"
        "    def _check(self, operation, path):\n"
        "        if not _native.can_access_filesystem(str(operation), str(path)):\n"
        "            raise PermissionError('PyraQt Python file system access is disabled')\n"
        "    def read_text(self, path):\n"
        "        self._check('read_text', path)\n"
        "        with open(path, 'r', encoding='utf-8') as fh:\n"
        "            return fh.read()\n"
        "    def write_text(self, path, text):\n"
        "        self._check('write_text', path)\n"
        "        with open(path, 'w', encoding='utf-8') as fh:\n"
        "            fh.write(str(text))\n"
        "        return path\n"
        "\n"
        "class _Log:\n"
        "    def info(self, text):\n"
        "        _native.log('info', str(text))\n"
        "    def warn(self, text):\n"
        "        _native.log('warn', str(text))\n"
        "    def error(self, text):\n"
        "        _native.log('error', str(text))\n"
        "\n"
        "_command_callbacks = {}\n"
        "class _Commands:\n"
        "    def register(self, id, callback=None, title=None, description='', keywords=None):\n"
        "        command_id = str(id)\n"
        "        if callable(callback):\n"
        "            _command_callbacks[command_id] = callback\n"
        "        elif title is None and callback is not None:\n"
        "            title = callback\n"
        "        _native.register_command(command_id, str(title or command_id), str(description), list(keywords or []))\n"
        "    def run(self, id):\n"
        "        callback = _command_callbacks.get(str(id))\n"
        "        if callback is None:\n"
        "            return None\n"
        "        return callback()\n"
        "\n"
        "class _Macros:\n"
        "    def load(self, code):\n"
        "        _native.load_macro(str(code))\n"
        "    def trigger(self, name):\n"
        "        _native.trigger_macro(str(name))\n"
        "\n"
        "class _Expressions:\n"
        "    def register(self, name, code):\n"
        "        _native.register_expression(str(name), str(code))\n"
        "    def evaluate(self, name, args=None):\n"
        "        _native.evaluate_expression(str(name), list(args or []))\n"
        "\n"
        "class _Processing:\n"
        "    def register(self, id, code):\n"
        "        _native.register_processing(str(id), str(code))\n"
        "    def run(self, id, parameters=None):\n"
        "        _native.run_processing(str(id), dict(parameters or {}))\n"
        "\n"
        "class _Noop:\n"
        "    def __getattr__(self, name):\n"
        "        def _missing(*args, **kwargs):\n"
        "            return None\n"
        "        return _missing\n"
        "\n"
        "class _Iface:\n"
        "    def __init__(self):\n"
        "        self.app = app\n"
        "        self.ui = ui\n"
        "        self.fs = fs\n"
        "        self.log = log\n"
        "        self.commands = commands\n"
        "        self.workspace = workspace\n"
        "        self.cad = cad\n"
        "        self.plugins = plugins\n"
        "        self.processing = processing\n"
        "        self.macros = macros\n"
        "        self.expressions = expressions\n"
        "    def mainWindow(self):\n"
        "        return None\n"
        "    def showMessage(self, text):\n"
        "        return self.ui.show_message(text)\n"
        "    def setStatus(self, text):\n"
        "        return self.ui.set_status(text)\n"
        "    def registerCommand(self, *args, **kwargs):\n"
        "        return self.commands.register(*args, **kwargs)\n"
        "\n"
        "app = _App()\n"
        "ui = _Ui()\n"
        "fs = _Fs()\n"
        "log = _Log()\n"
        "commands = _Commands()\n"
        "workspace = _Noop()\n"
        "cad = _Noop()\n"
        "plugins = _Noop()\n"
        "processing = _Processing()\n"
        "macros = _Macros()\n"
        "expressions = _Expressions()\n"
        "iface = _Iface()\n"
        "\n"
        "def install_streams():\n"
        "    import sys\n"
        "    sys.stdout = _Stream(_native.emit_stdout)\n"
        "    sys.stderr = _Stream(_native.emit_stderr)\n"
        "\n");

    if (!writeTextFile(QDir(packagePath).filePath(QStringLiteral("__init__.py")), module)) {
        return false;
    }

    if (!configureSysPath(QStringList{rootPath}, {})) {
        return false;
    }

    ScriptResult importResult = runString(QStringLiteral(
        "import pyra\n"
        "pyra.install_streams()\n"
        "iface = pyra.iface\n"
        "app = pyra.app\n"));
    return importResult.success;
}

bool PythonRuntimeManager::configureSysPath(const QStringList &extraPythonPaths, const QStringList &extraPluginPaths)
{
    PyObject *sysPath = PySys_GetObject("path");
    if (sysPath == nullptr || !PyList_Check(sysPath)) {
        return false;
    }

    QStringList paths = defaultPythonPaths();
    paths.append(extraPythonPaths);
    paths.append(pluginPaths());
    paths.append(extraPluginPaths);

    for (const QString &path : paths) {
        if (path.isEmpty()) {
            continue;
        }
        QDir dir(path);
        const QString cleanPath = QDir::cleanPath(dir.absolutePath());
        PyObject *pyPath = PyUnicode_FromString(cleanPath.toUtf8().constData());
        if (pyPath == nullptr) {
            continue;
        }
        if (PySequence_Contains(sysPath, pyPath) == 0) {
            PyList_Insert(sysPath, 0, pyPath);
        }
        Py_DECREF(pyPath);
    }
    return true;
}

QString PythonRuntimeManager::pyObjectToString(PyObject *object) const
{
    if (object == nullptr || object == Py_None) {
        return {};
    }
    PyObject *stringObject = PyObject_Str(object);
    if (stringObject == nullptr) {
        return {};
    }
    const QString result = QString::fromUtf8(PyUnicode_AsUTF8(stringObject));
    Py_DECREF(stringObject);
    return result;
}

QStringList PythonRuntimeManager::defaultPythonPaths() const
{
    QStringList paths;
    paths << pythonPath();
    paths << QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../python"));
    paths << QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../python"));
    return paths;
}

QStringList PythonRuntimeManager::defaultPluginPaths() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList paths;
    paths << QDir(appDir).filePath(QStringLiteral("plugins/python"));
    paths << QDir(appDir).filePath(QStringLiteral("../plugins/python"));
    paths << QDir(appDir).filePath(QStringLiteral("../../plugins/python"));
    paths << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../plugins/python"));
    return paths;
}

void PythonRuntimeManager::emitStdoutFromPython(const QString &output)
{
    emit stdoutReceived(output);
}

void PythonRuntimeManager::emitStderrFromPython(const QString &output)
{
    emit stderrReceived(output);
}

void PythonRuntimeManager::emitStatusFromPython(const QString &message)
{
    emit statusMessageRequested(message);
}

void PythonRuntimeManager::emitDialogFromPython(const QString &message)
{
    emit dialogMessageRequested(message);
}

void PythonRuntimeManager::emitLogFromPython(const QString &level, const QString &message)
{
    emit logMessageRequested(level, message);
}

void PythonRuntimeManager::emitCommandRegistrationFromPython(
    const QString &id,
    const QString &title,
    const QString &description,
    const QStringList &keywords)
{
    emit commandRegistrationRequested(m_registrationOwnerId, id, title, description, keywords);
}

bool PythonRuntimeManager::isFileSystemAccessAllowedFromPython(const QString &operation, const QString &path)
{
    const bool allowed = fileSystemAccessEnabled();
    if (!allowed) {
        emit filesystemAccessViolation(operation, path);
    }
    return allowed;
}

void PythonRuntimeManager::emitMacroLoadFromPython(const QString &code)
{
    emit macroLoadRequested(code);
}

void PythonRuntimeManager::emitMacroTriggerFromPython(const QString &name)
{
    emit macroTriggerRequested(name);
}

void PythonRuntimeManager::emitExpressionRegistrationFromPython(const QString &name, const QString &code)
{
    emit expressionRegistrationRequested(m_registrationOwnerId, name, code);
}

void PythonRuntimeManager::emitExpressionEvaluationFromPython(const QString &name, const QStringList &arguments)
{
    emit expressionEvaluationRequested(name, arguments);
}

void PythonRuntimeManager::emitProcessingRegistrationFromPython(const QString &id, const QString &code)
{
    emit processingRegistrationRequested(m_registrationOwnerId, id, code);
}

void PythonRuntimeManager::emitProcessingRunFromPython(const QString &id, const QVariantMap &parameters)
{
    emit processingRunRequested(id, parameters);
}

} // namespace pyraqt::core
