#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/modeling/model_import_manager.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/update/update_manager.h"
#include "core/workspace/workspace_manager.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {

class WorkspaceServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void workspaceDefaults();
    void recentFilesAreDeduplicatedAndTrimmed();
    void recentFilesCanBeRenamedAndRemoved();
    void sessionFiltersMissingFiles();
    void sessionKeepsSupportedModelFiles();
    void runtimeAndUpdateSettingsPersist();
};

QString createFile(QTemporaryDir &dir, const QString &name)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write("print('hello')\n");
    file.close();
    return path;
}

QString createTextFile(QTemporaryDir &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(content);
    file.close();
    return path;
}

void WorkspaceServicesTest::workspaceDefaults()
{
    pyraqt::core::ConfigManager configManager;
    QCOMPARE(configManager.value(QStringLiteral("workspace.restore_last_session")).toBool(), true);
    QCOMPARE(configManager.value(QStringLiteral("workspace.max_recent_files")).toInt(), 10);
    QVERIFY(configManager.value(QStringLiteral("workspace.recent_files")).toStringList().isEmpty());
}

void WorkspaceServicesTest::recentFilesAreDeduplicatedAndTrimmed()
{
    pyraqt::core::ConfigManager configManager;
    configManager.setValue(QStringLiteral("workspace.max_recent_files"), 2);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString first = createFile(dir, QStringLiteral("first.py"));
    const QString second = createFile(dir, QStringLiteral("second.py"));
    const QString third = createFile(dir, QStringLiteral("third.py"));

    workspaceManager.addRecentFile(first);
    workspaceManager.addRecentFile(second);
    workspaceManager.addRecentFile(first);
    workspaceManager.addRecentFile(third);

    const QStringList recentFiles = workspaceManager.recentFiles();
    QCOMPARE(recentFiles.size(), 2);
    QCOMPARE(recentFiles.at(0), QFileInfo(third).canonicalFilePath());
    QCOMPARE(recentFiles.at(1), QFileInfo(first).canonicalFilePath());
}

void WorkspaceServicesTest::recentFilesCanBeRenamedAndRemoved()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString first = createFile(dir, QStringLiteral("first.py"));
    const QString second = createFile(dir, QStringLiteral("second.py"));
    const QString renamed = dir.filePath(QStringLiteral("renamed.py"));
    QVERIFY(QFile::rename(first, renamed));

    workspaceManager.addRecentFile(first);
    workspaceManager.addRecentFile(second);
    workspaceManager.replaceRecentFilePath(first, renamed);

    QStringList recentFiles = workspaceManager.recentFiles();
    QCOMPARE(recentFiles.size(), 2);
    QCOMPARE(recentFiles.at(0), QFileInfo(renamed).canonicalFilePath());
    QCOMPARE(recentFiles.at(1), QFileInfo(second).canonicalFilePath());

    workspaceManager.removeRecentFile(second);
    recentFiles = workspaceManager.recentFiles();
    QCOMPARE(recentFiles.size(), 1);
    QCOMPARE(recentFiles.first(), QFileInfo(renamed).canonicalFilePath());
}

void WorkspaceServicesTest::sessionFiltersMissingFiles()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString existing = createFile(dir, QStringLiteral("existing.py"));
    pyraqt::core::WorkspaceSession session;
    session.openFiles = QStringList{existing, dir.filePath(QStringLiteral("missing.py"))};
    session.activeFile = existing;
    session.recentFiles = session.openFiles;
    session.fileBrowserRoot = dir.path();

    workspaceManager.saveSession(session);
    const pyraqt::core::WorkspaceSession restored = workspaceManager.restoreSession();

    QCOMPARE(restored.openFiles.size(), 1);
    QCOMPARE(restored.openFiles.first(), QFileInfo(existing).canonicalFilePath());
    QCOMPARE(restored.activeFile, QFileInfo(existing).canonicalFilePath());
    QCOMPARE(restored.fileBrowserRoot, dir.path());
}

void WorkspaceServicesTest::sessionKeepsSupportedModelFiles()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    pyraqt::core::ModelImportManager importManager;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString brepPath = createTextFile(dir, QStringLiteral("shape.brep"), QByteArray("BREP placeholder\n"));
    QVERIFY(importManager.isSupportedFile(brepPath));

    pyraqt::core::WorkspaceSession session;
    session.openFiles = QStringList{brepPath};
    session.activeFile = brepPath;
    session.recentFiles = QStringList{brepPath};
    session.fileBrowserRoot = dir.path();

    workspaceManager.saveSession(session);
    const pyraqt::core::WorkspaceSession restored = workspaceManager.restoreSession();

    QCOMPARE(restored.openFiles.size(), 1);
    QCOMPARE(restored.openFiles.first(), QFileInfo(brepPath).canonicalFilePath());
    QCOMPARE(restored.activeFile, QFileInfo(brepPath).canonicalFilePath());
}

void WorkspaceServicesTest::runtimeAndUpdateSettingsPersist()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::UpdateManager updateManager(configManager, logManager);

    runtimeManager.setInterpreterPath(QStringLiteral("python3"));
    runtimeManager.setExecutionTimeoutMs(42000);
    updateManager.setAutoCheckEnabled(true);
    updateManager.setChannel(QStringLiteral("preview"));

    QCOMPARE(configManager.value(QStringLiteral("python.interpreter_path")).toString(), QStringLiteral("python3"));
    QCOMPARE(configManager.value(QStringLiteral("python.execution_timeout_ms")).toInt(), 42000);
    QCOMPARE(configManager.value(QStringLiteral("updates.auto_check")).toBool(), true);
    QCOMPARE(configManager.value(QStringLiteral("updates.channel")).toString(), QStringLiteral("preview"));
}

} // namespace

QObject *createWorkspaceServicesTest()
{
    return new WorkspaceServicesTest();
}

#include "test_workspace_services.moc"
