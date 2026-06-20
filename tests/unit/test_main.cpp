#include <QApplication>
#include <QStandardPaths>
#include <QTest>

QObject *createCoreServicesTest();
QObject *createScriptingServicesTest();
QObject *createPluginServicesTest();
QObject *createRuntimeServicesTest();
QObject *createUiThemeTest();
QObject *createWorkspaceServicesTest();

int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("PyraQt"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("pyraqt.local"));
    QCoreApplication::setApplicationName(QStringLiteral("PyraQtTests"));

    QApplication app(argc, argv);
    Q_INIT_RESOURCE(pyraqt_resources);

    int status = 0;
    QObject *coreServicesTest = createCoreServicesTest();
    QObject *scriptingServicesTest = createScriptingServicesTest();
    QObject *pluginServicesTest = createPluginServicesTest();
    QObject *runtimeServicesTest = createRuntimeServicesTest();
    QObject *uiThemeTest = createUiThemeTest();
    QObject *workspaceServicesTest = createWorkspaceServicesTest();
    status |= QTest::qExec(coreServicesTest, argc, argv);
    status |= QTest::qExec(scriptingServicesTest, argc, argv);
    status |= QTest::qExec(pluginServicesTest, argc, argv);
    status |= QTest::qExec(runtimeServicesTest, argc, argv);
    status |= QTest::qExec(uiThemeTest, argc, argv);
    status |= QTest::qExec(workspaceServicesTest, argc, argv);
    delete coreServicesTest;
    delete scriptingServicesTest;
    delete pluginServicesTest;
    delete runtimeServicesTest;
    delete uiThemeTest;
    delete workspaceServicesTest;
    return status;
}
