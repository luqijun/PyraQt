#include "core/scripting/pyra_api_bridge.h"

#include "core/logging/log_manager.h"
#include "core/scripting/python_runtime_manager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>

namespace pyraqt::core {
namespace {

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

} // namespace

PyraApiBridge::PyraApiBridge(PythonRuntimeManager &runtimeManager, LogManager &logManager, QObject *parent)
    : QObject(parent)
    , m_runtimeManager(runtimeManager)
    , m_logManager(logManager)
{
}

QString PyraApiBridge::bridgeRootPath() const
{
    return QDir(dataRootPath()).filePath(QStringLiteral("bridge"));
}

bool PyraApiBridge::ensureBridgeFiles() const
{
    QDir bridgeRoot(bridgeRootPath());
    if (!bridgeRoot.exists() && !bridgeRoot.mkpath(QStringLiteral("."))) {
        return false;
    }

    QDir packageDir(packageDirectory());
    if (!packageDir.exists() && !bridgeRoot.mkpath(QStringLiteral("pyra"))) {
        return false;
    }

    const bool moduleWritten = writeTextFile(packageDir.filePath(QStringLiteral("__init__.py")), moduleScript());
    const bool bootstrapWritten = writeTextFile(bootstrapFilePath(), bootstrapScript());
    if (!moduleWritten || !bootstrapWritten) {
        return false;
    }

    return true;
}

QString PyraApiBridge::bootstrapFilePath() const
{
    return QDir(bridgeRootPath()).filePath(QStringLiteral("pyra_bootstrap.py"));
}

QString PyraApiBridge::dataRootPath() const
{
    return ::pyraqt::core::dataRootPath();
}

QString PyraApiBridge::packageDirectory() const
{
    return QDir(bridgeRootPath()).filePath(QStringLiteral("pyra"));
}

QString PyraApiBridge::bootstrapScript() const
{
    return QStringLiteral(
        "import os\n"
        "import runpy\n"
        "import sys\n"
        "bridge_root = os.path.dirname(__file__)\n"
        "if bridge_root not in sys.path:\n"
        "    sys.path.insert(0, bridge_root)\n"
        "if len(sys.argv) < 2:\n"
        "    raise SystemExit('Missing target script path')\n"
        "target_script = sys.argv[1]\n"
        "sys.argv = [target_script] + sys.argv[2:]\n"
        "runpy.run_path(target_script, run_name='__main__')\n");
}

QString PyraApiBridge::moduleScript() const
{
    return QStringLiteral(
        "import json\n"
        "import os\n"
        "import pathlib\n"
        "import sys\n"
        "\n"
        "_COMMAND_PATH = os.environ.get('PYRAQT_BRIDGE_COMMAND_FILE')\n"
        "_APP_NAME = os.environ.get('PYRAQT_APP_NAME', 'PyraQt')\n"
        "_APP_VERSION = os.environ.get('PYRAQT_APP_VERSION', '0.1.0')\n"
        "_PYTHON_PATH = os.environ.get('PYRAQT_PYTHON_PATH', sys.executable)\n"
        "\n"
        "def _emit(kind, payload):\n"
        "    if not _COMMAND_PATH:\n"
        "        return\n"
        "    entry = {'kind': kind, 'payload': payload}\n"
        "    with open(_COMMAND_PATH, 'a', encoding='utf-8') as fh:\n"
        "        fh.write(json.dumps(entry, ensure_ascii=False) + '\\n')\n"
        "\n"
        "class _App:\n"
        "    @property\n"
        "    def name(self):\n"
        "        return _APP_NAME\n"
        "    @property\n"
        "    def version(self):\n"
        "        return _APP_VERSION\n"
        "    @property\n"
        "    def interpreter_path(self):\n"
        "        return _PYTHON_PATH\n"
        "\n"
        "class _Ui:\n"
        "    def show_message(self, text):\n"
        "        _emit('show_message', {'text': str(text)})\n"
        "    def set_status(self, text):\n"
        "        _emit('set_status', {'text': str(text)})\n"
        "\n"
        "class _Fs:\n"
        "    def read_text(self, path):\n"
        "        return pathlib.Path(path).read_text(encoding='utf-8')\n"
        "    def write_text(self, path, text):\n"
        "        pathlib.Path(path).write_text(str(text), encoding='utf-8')\n"
        "        return path\n"
        "\n"
        "class _Log:\n"
        "    def info(self, text):\n"
        "        _emit('log', {'level': 'info', 'text': str(text)})\n"
        "    def warn(self, text):\n"
        "        _emit('log', {'level': 'warn', 'text': str(text)})\n"
        "    def error(self, text):\n"
        "        _emit('log', {'level': 'error', 'text': str(text)})\n"
        "\n"
        "app = _App()\n"
        "ui = _Ui()\n"
        "fs = _Fs()\n"
        "log = _Log()\n");
}

} // namespace pyraqt::core
