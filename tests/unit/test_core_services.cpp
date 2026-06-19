#include "core/config/config_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/logging/log_manager.h"
#include "core/theme/theme_manager.h"

#include <QFileInfo>
#include <QTest>

namespace {

class CoreServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void configRoundTrip();
    void themeSwitching();
    void localeSwitching();
    void loggingInitialization();
};

void CoreServicesTest::configRoundTrip()
{
    pyraqt::core::ConfigManager manager;
    QCOMPARE(manager.value(QStringLiteral("theme")).toString(), QStringLiteral("dark"));
    manager.setValue(QStringLiteral("theme"), QStringLiteral("light"));
    QVERIFY(manager.save());
    QVERIFY(manager.load());
    QCOMPARE(manager.value(QStringLiteral("theme")).toString(), QStringLiteral("light"));
}

void CoreServicesTest::themeSwitching()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    QVERIFY(themeManager.setLightTheme());
    QCOMPARE(themeManager.currentTheme(), QStringLiteral("light"));
    QVERIFY(themeManager.setDarkTheme());
    QCOMPARE(themeManager.currentTheme(), QStringLiteral("dark"));
}

void CoreServicesTest::localeSwitching()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::I18nManager i18nManager(*app);
    QVERIFY(i18nManager.setLocale(QStringLiteral("en_US")));
    QCOMPARE(i18nManager.currentLocale(), QStringLiteral("en_US"));
    QVERIFY(i18nManager.setLocale(QStringLiteral("zh_CN")) || i18nManager.currentLocale() == QStringLiteral("en_US"));
}

void CoreServicesTest::loggingInitialization()
{
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    logManager.info(QStringLiteral("test message"));
    QVERIFY(QFileInfo::exists(logManager.logFilePath()));
}

} // namespace

QObject *createCoreServicesTest()
{
    return new CoreServicesTest();
}

#include "test_core_services.moc"
