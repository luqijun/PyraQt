#include "core/theme/theme_manager.h"
#include "core/modeling/model_import_manager.h"
#include "ui/common/file_dialog_utils.h"
#include "ui/editor/editor_workspace_widget.h"
#include "ui/editor/script_editor_widget.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include <memory>

namespace {

class UiThemeTest final : public QObject {
    Q_OBJECT

private slots:
    void themeManagerEmitsThemeChanged();
    void editorAppliesThemeAndTracksChanges();
    void themedFileDialogUsesQtDialogSettings();
};

void UiThemeTest::themeManagerEmitsThemeChanged()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    QSignalSpy spy(&themeManager, &pyraqt::core::ThemeManager::themeChanged);

    QVERIFY(themeManager.setDarkTheme());
    QCOMPARE(themeManager.currentTheme(), QStringLiteral("dark"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("dark"));

    QVERIFY(themeManager.setLightTheme());
    QCOMPARE(themeManager.currentTheme(), QStringLiteral("light"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("light"));
}

void UiThemeTest::editorAppliesThemeAndTracksChanges()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QVERIFY(themeManager.setDarkTheme());
    pyraqt::ui::ScriptEditorWidget *editor = workspace.newDocument();
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->appliedTheme(), QStringLiteral("dark"));

    QVERIFY(themeManager.setLightTheme());
    QCOMPARE(editor->appliedTheme(), QStringLiteral("light"));
}

void UiThemeTest::themedFileDialogUsesQtDialogSettings()
{
    const pyraqt::ui::FileDialogRequest request{
        QStringLiteral("Open File"),
        QStringLiteral("/tmp"),
        QStringLiteral("Python Files (*.py);;All Files (*)"),
        QFileDialog::ExistingFile,
        QFileDialog::AcceptOpen,
    };

    std::unique_ptr<QFileDialog> dialog(pyraqt::ui::createThemedFileDialog(request));
    QVERIFY(dialog->testOption(QFileDialog::DontUseNativeDialog));
    QCOMPARE(dialog->fileMode(), QFileDialog::ExistingFile);
    QCOMPARE(dialog->acceptMode(), QFileDialog::AcceptOpen);
    QCOMPARE(dialog->directory().absolutePath(), QStringLiteral("/tmp"));
    const QStringList expectedFilters{QStringLiteral("Python Files (*.py)"), QStringLiteral("All Files (*)")};
    QCOMPARE(dialog->nameFilters(), expectedFilters);
}

} // namespace

QObject *createUiThemeTest()
{
    return new UiThemeTest();
}

#include "test_ui_theme.moc"
