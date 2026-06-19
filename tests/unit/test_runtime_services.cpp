#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/runtime/crash_recovery_manager.h"
#include "core/update/update_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {

class RuntimeServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void updateCheckWritesTimestamp();
    void crashRecoveryDetectsAbnormalExit();
};

void RuntimeServicesTest::updateCheckWritesTimestamp()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());

    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    QSignalSpy finishedSpy(&updateManager, &pyraqt::core::UpdateManager::updateCheckFinished);

    const pyraqt::core::UpdateCheckResult result = updateManager.checkForUpdates(true);

    QCOMPARE(result.status, pyraqt::core::UpdateStatus::NotConfigured);
    QVERIFY(!result.message.isEmpty());
    QVERIFY(!updateManager.lastCheckTime().isEmpty());
    QCOMPARE(finishedSpy.count(), 1);
}

void RuntimeServicesTest::crashRecoveryDetectsAbnormalExit()
{
    QTemporaryDir dataDir;
    QVERIFY(dataDir.isValid());
    qputenv("PYRAQT_DATA_DIR", QFile::encodeName(dataDir.path()));

    pyraqt::core::LogManager firstLogManager;
    QVERIFY(firstLogManager.initialize());

    {
        pyraqt::core::ConfigManager configManager;
        pyraqt::core::CrashRecoveryManager recoveryManager(configManager, firstLogManager);
        recoveryManager.markAppStarted();
        QVERIFY(!recoveryManager.didPreviousRunCrash());
        QCOMPARE(configManager.value(QStringLiteral("runtime.last_exit_clean")).toBool(), false);
    }

    pyraqt::core::LogManager secondLogManager;
    QVERIFY(secondLogManager.initialize());
    pyraqt::core::ConfigManager secondConfigManager;
    QVERIFY(secondConfigManager.load());
    pyraqt::core::CrashRecoveryManager secondRecoveryManager(secondConfigManager, secondLogManager);
    secondRecoveryManager.markAppStarted();

    QVERIFY(secondRecoveryManager.didPreviousRunCrash());
    QVERIFY(QFileInfo::exists(secondRecoveryManager.crashLogPath()));

    secondRecoveryManager.markAppExitedNormally();
    QCOMPARE(secondConfigManager.value(QStringLiteral("runtime.last_exit_clean")).toBool(), true);
}

} // namespace

QObject *createRuntimeServicesTest()
{
    return new RuntimeServicesTest();
}

#include "test_runtime_services.moc"
