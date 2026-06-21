#include "core/theme/theme_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/modeling/model_import_manager.h"
#include "core/config/config_manager.h"
#include "core/command/command_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/runtime/crash_recovery_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_completion_provider.h"
#include "core/scripting/python_feature_manager.h"
#include "core/scripting/python_runner.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"
#include "core/update/update_manager.h"
#include "core/workspace/workspace_manager.h"
#include "ui/common/file_dialog_utils.h"
#include "ui/common/python_completion_line_edit.h"
#include "ui/common/python_completion_text_edit.h"
#include "ui/dialogs/command_palette_dialog.h"
#include "ui/dialogs/python_tools_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/editor/editor_placeholder_widget.h"
#include "ui/editor/editor_workspace_widget.h"
#include "ui/editor/script_editor_widget.h"
#include "ui/common/icon_utils.h"
#include "ui/mainwindow/main_window.h"
#include "ui/panels/files/file_browser_panel.h"
#include "ui/panels/plugins/plugin_manager_panel.h"
#include "ui/panels/properties/model_properties_panel.h"
#include "ui/panels/python/python_console_widget.h"

#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileSystemModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QSizePolicy>
#include <QToolButton>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeView>
#include <QStandardPaths>
#include <memory>

namespace {

class UiThemeTest final : public QObject {
    Q_OBJECT

private slots:
    void themeManagerEmitsThemeChanged();
    void editorAppliesThemeAndTracksChanges();
    void workspaceRenamesOpenPathsAndClosesByPath();
    void workspaceClosesOtherEditors();
    void workspaceClosesEditorsToRight();
    void workspaceCloseAllEditorsKeepsUntitledTab();
    void workspaceStopsBatchCloseWhenConfirmationRejected();
    void newDocumentDefaultsToPlainText();
    void workspaceOpensPlainTextAndBinaryFiles();
    void editorModeFollowsFilePathSuffix();
    void pythonCompletionProviderIncludesPyraApi();
    void pythonCompletionProviderParsesDottedPrefixes();
    void editorConfiguresCodeCompletion();
    void consoleLineEditTriggersDottedCompletion();
    void consoleEditorTriggersDottedCompletion();
    void consoleConfiguresCodeCompletionAndRuntimeGlobals();
    void mainWindowToolbarShowsIconsOnlyAndRefreshesOnThemeChange();
    void localeSwitchRefreshesOpenWidgetsAndCommands();
    void modelViewActionsUseStableViewKeys();
    void settingsDialogShowsResolvedInterpreterPath();
    void mainWindowConsoleAndLogDocksCanBeCleared();
    void mainWindowPropertiesDockTracksCurrentDocumentKind();
    void modelPropertiesPanelUsesFixedMinimumWidthAndTooltips();
    void fileBrowserPanelProvidesTooltips();
    void pluginManagerPanelProvidesTooltips();
    void themedFileDialogUsesQtDialogSettings();
    void themedFileDialogUsesRequestedDefaultSuffix();
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

void UiThemeTest::workspaceRenamesOpenPathsAndClosesByPath()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString scriptPath = dir.filePath(QStringLiteral("script.py"));
    QFile scriptFile(scriptPath);
    QVERIFY(scriptFile.open(QIODevice::WriteOnly | QIODevice::Text));
    scriptFile.write("print('hello')\n");
    scriptFile.close();

    const QString modelPath = dir.filePath(QStringLiteral("shape.brep"));
    QFile modelFile(modelPath);
    QVERIFY(modelFile.open(QIODevice::WriteOnly | QIODevice::Text));
    modelFile.write("BREP placeholder\n");
    modelFile.close();

    QVERIFY(workspace.openPath(scriptPath));
    QVERIFY(workspace.openPath(modelPath));

    const QString renamedScriptPath = dir.filePath(QStringLiteral("renamed_script.py"));
    const QString renamedModelPath = dir.filePath(QStringLiteral("renamed_shape.brep"));

    QVERIFY(workspace.renameOpenPath(scriptPath, renamedScriptPath));
    QVERIFY(workspace.renameOpenPath(modelPath, renamedModelPath));
    QVERIFY(workspace.hasOpenPath(renamedScriptPath));
    QVERIFY(workspace.hasOpenPath(renamedModelPath));
    QVERIFY(!workspace.hasOpenPath(scriptPath));
    QVERIFY(!workspace.hasOpenPath(modelPath));
    QVERIFY(workspace.openFilePaths().contains(renamedScriptPath));
    QVERIFY(workspace.openFilePaths().contains(renamedModelPath));

    pyraqt::ui::ScriptEditorWidget *currentEditor = workspace.currentEditor();
    if (currentEditor != nullptr && currentEditor->currentFilePath() == renamedScriptPath) {
        QVERIFY(!currentEditor->isModified());
    }

    QVERIFY(workspace.closePath(renamedModelPath));
    QVERIFY(!workspace.hasOpenPath(renamedModelPath));
    QVERIFY(workspace.closePath(renamedScriptPath));
    QCOMPARE(workspace.editorCount(), 1);
    QCOMPARE(workspace.currentFilePath(), QString());
}

void UiThemeTest::workspaceClosesOtherEditors()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString firstPath = dir.filePath(QStringLiteral("first.py"));
    const QString secondPath = dir.filePath(QStringLiteral("second.py"));
    const QString thirdPath = dir.filePath(QStringLiteral("third.py"));
    for (const QString &path : {firstPath, secondPath, thirdPath}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("print('hello')\n");
        file.close();
        QVERIFY(workspace.openPath(path));
    }

    QVERIFY(workspace.openPath(secondPath));
    QCOMPARE(workspace.currentFilePath(), secondPath);

    workspace.closeOtherEditors();

    QCOMPARE(workspace.editorCount(), 1);
    QCOMPARE(workspace.currentFilePath(), secondPath);
    QVERIFY(workspace.hasOpenPath(secondPath));
    QVERIFY(!workspace.hasOpenPath(firstPath));
    QVERIFY(!workspace.hasOpenPath(thirdPath));
}

void UiThemeTest::workspaceClosesEditorsToRight()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString firstPath = dir.filePath(QStringLiteral("first.py"));
    const QString secondPath = dir.filePath(QStringLiteral("second.py"));
    const QString thirdPath = dir.filePath(QStringLiteral("third.py"));
    const QString fourthPath = dir.filePath(QStringLiteral("fourth.py"));
    for (const QString &path : {firstPath, secondPath, thirdPath, fourthPath}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("print('hello')\n");
        file.close();
        QVERIFY(workspace.openPath(path));
    }

    QVERIFY(workspace.openPath(secondPath));
    QCOMPARE(workspace.currentFilePath(), secondPath);
    QVERIFY(workspace.hasEditorsToRight());

    workspace.closeEditorsToRight();

    QCOMPARE(workspace.editorCount(), 2);
    QCOMPARE(workspace.currentFilePath(), secondPath);
    QVERIFY(workspace.hasOpenPath(firstPath));
    QVERIFY(workspace.hasOpenPath(secondPath));
    QVERIFY(!workspace.hasOpenPath(thirdPath));
    QVERIFY(!workspace.hasOpenPath(fourthPath));
    QVERIFY(!workspace.hasEditorsToRight());
}

void UiThemeTest::workspaceCloseAllEditorsKeepsUntitledTab()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString scriptPath = dir.filePath(QStringLiteral("script.py"));
    QFile file(scriptPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("print('hello')\n");
    file.close();

    QVERIFY(workspace.openPath(scriptPath));
    QVERIFY(workspace.closeAllEditors());

    QCOMPARE(workspace.editorCount(), 1);
    QCOMPARE(workspace.currentFilePath(), QString());
    QVERIFY(workspace.currentEditor() != nullptr);
}

void UiThemeTest::workspaceStopsBatchCloseWhenConfirmationRejected()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString firstPath = dir.filePath(QStringLiteral("first.py"));
    const QString secondPath = dir.filePath(QStringLiteral("second.py"));
    const QString thirdPath = dir.filePath(QStringLiteral("third.py"));
    for (const QString &path : {firstPath, secondPath, thirdPath}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("print('hello')\n");
        file.close();
        QVERIFY(workspace.openPath(path));
    }

#if PYRAQT_HAS_QSCINTILLA
    QVERIFY(workspace.openPath(secondPath));
    pyraqt::ui::ScriptEditorWidget *editor = workspace.currentEditor();
    QVERIFY(editor != nullptr);
    editor->setTextForTesting(QStringLiteral("print('modified')\n"));
    QVERIFY(editor->isModified());

    int confirmationCount = 0;
    connect(&workspace, &pyraqt::ui::EditorWorkspaceWidget::requestCloseConfirmation, &workspace,
        [&confirmationCount](const QString &, const QString &, bool *accepted) {
            ++confirmationCount;
            QVERIFY(accepted != nullptr);
            *accepted = false;
        });

    workspace.closeAllEditors();

    QCOMPARE(confirmationCount, 1);
    QCOMPARE(workspace.editorCount(), 2);
    QVERIFY(workspace.hasOpenPath(firstPath));
    QVERIFY(workspace.hasOpenPath(secondPath));
    QVERIFY(!workspace.hasOpenPath(thirdPath));
#else
    QSKIP("Modified-state close confirmation requires the QScintilla editor backend.");
#endif
}

void UiThemeTest::newDocumentDefaultsToPlainText()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::ui::ScriptEditorWidget editor(&runtimeManager);

    editor.newDocument();

    QCOMPARE(editor.documentMode(), pyraqt::ui::ScriptEditorWidget::DocumentMode::PlainText);
    QVERIFY(!editor.isPythonDocument());
    QCOMPARE(editor.currentText(), QString());
#if PYRAQT_HAS_QSCINTILLA
    QVERIFY(!editor.codeCompletionEnabled());
    QVERIFY(!editor.dotCompletionEnabled());
#endif
}

void UiThemeTest::workspaceOpensPlainTextAndBinaryFiles()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::ui::EditorWorkspaceWidget workspace(themeManager, modelImportManager);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString readmePath = dir.filePath(QStringLiteral("README"));
    QFile readmeFile(readmePath);
    QVERIFY(readmeFile.open(QIODevice::WriteOnly | QIODevice::Text));
    readmeFile.write("plain text content\n");
    readmeFile.close();

    const QString binaryPath = dir.filePath(QStringLiteral("archive.bin"));
    QFile binaryFile(binaryPath);
    QVERIFY(binaryFile.open(QIODevice::WriteOnly));
    binaryFile.write(QByteArray("abc\0def", 7));
    binaryFile.close();

    QVERIFY(workspace.openPath(readmePath));
    QCOMPARE(workspace.currentDocumentKind(), pyraqt::ui::EditorWorkspaceWidget::DocumentKind::Text);
    QVERIFY(workspace.currentEditor() != nullptr);
    QCOMPARE(workspace.currentEditor()->documentMode(), pyraqt::ui::ScriptEditorWidget::DocumentMode::PlainText);
    QVERIFY(!workspace.currentFileRunnable());

    QVERIFY(workspace.openPath(binaryPath));
    QCOMPARE(workspace.currentDocumentKind(), pyraqt::ui::EditorWorkspaceWidget::DocumentKind::PreviewUnavailable);
    QCOMPARE(workspace.currentFilePath(), binaryPath);
    QVERIFY(workspace.currentEditor() == nullptr);
    QVERIFY(!workspace.hasAvailableEditor());
    QVERIFY(!workspace.currentFileRunnable());
    QVERIFY(workspace.hasOpenPath(binaryPath));

    auto *placeholder = workspace.findChild<pyraqt::ui::EditorPlaceholderWidget *>();
    QVERIFY(placeholder != nullptr);
    QCOMPARE(placeholder->filePath(), binaryPath);
}

void UiThemeTest::editorModeFollowsFilePathSuffix()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::ui::ScriptEditorWidget editor(&runtimeManager);

    editor.newDocument();
    QCOMPARE(editor.documentMode(), pyraqt::ui::ScriptEditorWidget::DocumentMode::PlainText);

    const QString pythonPath = dir.filePath(QStringLiteral("example.py"));
    QVERIFY(editor.saveAs(pythonPath));
    QCOMPARE(editor.documentMode(), pyraqt::ui::ScriptEditorWidget::DocumentMode::Python);
    QVERIFY(editor.isPythonDocument());
#if PYRAQT_HAS_QSCINTILLA
    QVERIFY(editor.codeCompletionEnabled());
    QVERIFY(editor.dotCompletionEnabled());
#endif

    const QString textPath = dir.filePath(QStringLiteral("example.txt"));
    QVERIFY(editor.saveAs(textPath));
    QCOMPARE(editor.documentMode(), pyraqt::ui::ScriptEditorWidget::DocumentMode::PlainText);
    QVERIFY(!editor.isPythonDocument());
#if PYRAQT_HAS_QSCINTILLA
    QVERIFY(!editor.codeCompletionEnabled());
    QVERIFY(!editor.dotCompletionEnabled());
#endif
}

void UiThemeTest::pythonCompletionProviderIncludesPyraApi()
{
    pyraqt::core::PythonCompletionProvider provider;
    const QStringList completions = provider.staticCompletions();
    QVERIFY(completions.contains(QStringLiteral("print")));
    QVERIFY(completions.contains(QStringLiteral("pyra.ui.set_status")));
    QVERIFY(completions.contains(QStringLiteral("pyra.processing.run")));
    QVERIFY(completions.contains(QStringLiteral("iface.setStatus")));
}

void UiThemeTest::pythonCompletionProviderParsesDottedPrefixes()
{
    QCOMPARE(pyraqt::core::PythonCompletionProvider::prefixBeforeCursor(QStringLiteral("pyra."), 5), QStringLiteral("pyra."));
    QCOMPARE(pyraqt::core::PythonCompletionProvider::prefixBeforeCursor(QStringLiteral("pyra.ui."), 8), QStringLiteral("pyra.ui."));
    QCOMPARE(pyraqt::core::PythonCompletionProvider::prefixBeforeCursor(QStringLiteral("iface."), 6), QStringLiteral("iface."));
    QCOMPARE(
        pyraqt::core::PythonCompletionProvider::prefixBeforeCursor(QStringLiteral("print(pyra.ui."), 14),
        QStringLiteral("pyra.ui."));
    QCOMPARE(
        pyraqt::core::PythonCompletionProvider::objectExpressionForMemberPrefix(QStringLiteral("custom_symbol.")),
        QStringLiteral("custom_symbol"));
    QVERIFY(pyraqt::core::PythonCompletionProvider::prefixedMemberCompletions(
                QStringLiteral("custom_symbol"),
                {QStringLiteral("append")})
                .contains(QStringLiteral("custom_symbol.append")));
    const QStringList pyraUiMembers = pyraqt::core::PythonCompletionProvider::directMembersForObject(
        pyraqt::core::PythonCompletionProvider().staticCompletions(),
        QStringLiteral("pyra.ui"));
    QVERIFY(pyraUiMembers.contains(QStringLiteral("set_status")));
    QVERIFY(!pyraUiMembers.contains(QStringLiteral("append")));
}

void UiThemeTest::editorConfiguresCodeCompletion()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::ui::ScriptEditorWidget editor(&runtimeManager);
    editor.setCurrentFilePath(QStringLiteral("sample.py"));
#if PYRAQT_HAS_QSCINTILLA
    QVERIFY(editor.codeCompletionEnabled());
    QVERIFY(editor.dotCompletionEnabled());
    QVERIFY(editor.completionWords().contains(QStringLiteral("pyra.ui.set_status")));
    QVERIFY(editor.completionWords().contains(QStringLiteral("iface.setStatus")));
    QVERIFY(runtimeManager.runString(QStringLiteral("custom_list = []")).success);
    QVERIFY(runtimeManager.completeMembers(QStringLiteral("custom_list"), QStringLiteral("custom_list = []\n"))
                .contains(QStringLiteral("append")));
    QVERIFY(runtimeManager.completeMembers(QStringLiteral("pyra.ui"), QStringLiteral("import pyra\n"))
                .contains(QStringLiteral("set_status")));
    QVERIFY(runtimeManager.completeMembers(QStringLiteral("sys"), QStringLiteral("import sys\n"))
                .contains(QStringLiteral("path")));
    editor.setTextForTesting(QStringLiteral("import sys\nsys"));
    editor.triggerDotCompletionForTesting();
    QVERIFY(editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("path")));
    editor.typeMemberTextForTesting(QStringLiteral("pa"));
    QVERIFY(editor.currentText().contains(QStringLiteral("sys.pa")));
    QVERIFY(editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("path")));
    QVERIFY(!editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("pass")));
    editor.setTextForTesting(QStringLiteral("import sys\nsys.p"));
    editor.refreshMemberCompletionForTesting();
    QVERIFY(editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("path")));
    QVERIFY(!editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("pass")));
    editor.setTextForTesting(QStringLiteral("import sys\nsys.pa"));
    editor.refreshMemberCompletionForTesting();
    QVERIFY(editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("path")));
    QVERIFY(!editor.lastMemberCompletionWordsForTesting().contains(QStringLiteral("pass")));
    QVERIFY(pyraqt::core::PythonCompletionProvider::directMembersForObject(
                editor.completionWords(),
                QStringLiteral("pyra.ui"))
                .contains(QStringLiteral("set_status")));
    QVERIFY(!pyraqt::core::PythonCompletionProvider::directMembersForObject(
                 editor.completionWords(),
                 QStringLiteral("pyra.ui"))
                 .contains(QStringLiteral("append")));
#else
    QVERIFY(!editor.codeCompletionEnabled());
    QVERIFY(!editor.dotCompletionEnabled());
#endif
}

void UiThemeTest::consoleLineEditTriggersDottedCompletion()
{
    pyraqt::ui::PythonCompletionLineEdit input;
    input.setCompletionWords({QStringLiteral("custom_list")});
    input.setMemberCompletionProvider([](const QString &prefix) {
        return pyraqt::core::PythonCompletionProvider::prefixedMemberCompletions(
            pyraqt::core::PythonCompletionProvider::objectExpressionForMemberPrefix(prefix),
            {QStringLiteral("append"), QStringLiteral("clear")});
    });
    QTest::keyClicks(&input, QStringLiteral("custom_list."));
    QCOMPARE(input.completionPrefixForTesting(), QStringLiteral("custom_list."));
    QVERIFY(input.completionCandidatesForTesting().contains(QStringLiteral("custom_list.append")));
    QTest::keyClick(&input, Qt::Key_Tab);
    QCOMPARE(input.text(), QStringLiteral("custom_list.append"));
}

void UiThemeTest::consoleEditorTriggersDottedCompletion()
{
    pyraqt::ui::PythonCompletionTextEdit editor;
    editor.setCompletionWords({QStringLiteral("pyra.ui"), QStringLiteral("pyra.ui.set_status")});
    editor.setMemberCompletionProvider([](const QString &prefix, const QString &) {
        return pyraqt::core::PythonCompletionProvider::prefixedMemberCompletions(
            pyraqt::core::PythonCompletionProvider::objectExpressionForDottedPrefix(prefix),
            {QStringLiteral("append"), QStringLiteral("set_status")});
    });
    QTest::keyClicks(&editor, QStringLiteral("pyra.ui."));
    QCOMPARE(editor.completionPrefixForTesting(), QStringLiteral("pyra.ui."));
    QVERIFY(editor.completionWords().contains(QStringLiteral("pyra.ui.set_status")));
    QVERIFY(editor.applyCurrentCompletionForTesting());
    QVERIFY(editor.toPlainText().startsWith(QStringLiteral("pyra.ui.")));

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    const QString objectExpression = pyraqt::core::PythonCompletionProvider::objectExpressionForDottedPrefix(QStringLiteral("os.pat"));
    QStringList osMembers = runtimeManager.completeMembers(objectExpression, QStringLiteral("import os\n"));
    QStringList filtered;
    for (const QString &member : osMembers) {
        if (member.startsWith(QStringLiteral("pat"), Qt::CaseInsensitive)) {
            filtered.push_back(member);
        }
    }
    const QStringList osCandidates = pyraqt::core::PythonCompletionProvider::prefixedMemberCompletions(objectExpression, filtered);
    QVERIFY(osCandidates.contains(QStringLiteral("os.path")));
    QVERIFY(!osCandidates.contains(QStringLiteral("os.pop")));
}

void UiThemeTest::consoleConfiguresCodeCompletionAndRuntimeGlobals()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);

    QVERIFY(runner.runCode(QStringLiteral("custom_symbol = 123\ncustom_list = []")).success);
    pyraqt::ui::PythonConsoleWidget console(runtimeManager, executionManager);
    QVERIFY(console.inputCompletionEnabled());
    QVERIFY(console.editorCompletionEnabled());
    QVERIFY(console.completionWords().contains(QStringLiteral("pyra.processing.run")));
    QVERIFY(console.completionWords().contains(QStringLiteral("custom_symbol")));

    auto *input = console.findChild<QLineEdit *>();
    QVERIFY(input != nullptr);
    QTest::keyClicks(input, QStringLiteral("pyra."));
    QTest::qWait(20);
    QCOMPARE(console.inputCompletionPrefixForTesting(), QStringLiteral("pyra."));
    input->clear();
    QTest::keyClicks(input, QStringLiteral("custom_list."));
    QTest::qWait(20);
    QCOMPARE(console.inputCompletionPrefixForTesting(), QStringLiteral("custom_list."));
    QVERIFY(console.inputCompletionCandidatesForTesting().contains(QStringLiteral("custom_list.append")));
    input->clear();
    QTest::keyClicks(input, QStringLiteral("unknown_object."));
    QTest::qWait(20);
    QCOMPARE(console.inputCompletionPrefixForTesting(), QStringLiteral("unknown_object."));
    QVERIFY(console.inputCompletionCandidatesForTesting().contains(QStringLiteral("unknown_object.__class__")));
    const QStringList osPathCandidates = console.memberCompletionsForTesting(QStringLiteral("os.pat"), QStringLiteral("import os\n"));
    QVERIFY(osPathCandidates.contains(QStringLiteral("os.path")));
    QVERIFY(!osPathCandidates.contains(QStringLiteral("os.pop")));

    console.submitCommandForTesting(QStringLiteral("repl_value = 123"));
    QVERIFY(!console.outputTextForTesting().contains(QStringLiteral("[result] 123")));
    console.submitCommandForTesting(QStringLiteral("repl_value"));
    QVERIFY(console.outputTextForTesting().contains(QStringLiteral("[result] 123")));
    console.submitCommandForTesting(QStringLiteral("len([1, 2, 3])"));
    QVERIFY(console.outputTextForTesting().contains(QStringLiteral("[result] 3")));
    console.submitCommandForTesting(QStringLiteral("print('hello repl')"));
    QVERIFY(console.outputTextForTesting().contains(QStringLiteral("[stdout] hello repl")));
    QVERIFY(!console.outputTextForTesting().contains(QStringLiteral("[result] None")));

    QVERIFY(console.completionWords().contains(QStringLiteral("custom_symbol")));
    QVERIFY(console.actionButtonsHaveIcons());
    QVERIFY(!console.actionButtonsShowText());
}

void UiThemeTest::mainWindowToolbarShowsIconsOnlyAndRefreshesOnThemeChange()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::I18nManager i18nManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    pyraqt::core::CrashRecoveryManager crashRecoveryManager(configManager, logManager);

    pyraqt::ui::MainWindow window(configManager,
        logManager,
        modelImportManager,
        themeManager,
        i18nManager,
        runtimeManager,
        executionManager,
        commandManager,
        pluginManager,
        workspaceManager,
        updateManager,
        crashRecoveryManager);

    auto *fileToolBar = window.findChild<QToolBar *>(QStringLiteral("fileToolBar"));
    auto *scriptToolBar = window.findChild<QToolBar *>(QStringLiteral("scriptToolBar"));
    auto *workspaceToolBar = window.findChild<QToolBar *>(QStringLiteral("workspaceToolBar"));
    auto *viewToolBar = window.findChild<QToolBar *>(QStringLiteral("viewToolBar"));
    auto *selectionToolBar = window.findChild<QToolBar *>(QStringLiteral("selectionToolBar"));
    auto *themeToolBar = window.findChild<QToolBar *>(QStringLiteral("themeToolBar"));
    QVERIFY(fileToolBar != nullptr);
    QVERIFY(scriptToolBar != nullptr);
    QVERIFY(workspaceToolBar != nullptr);
    QVERIFY(viewToolBar != nullptr);
    QVERIFY(selectionToolBar != nullptr);
    QVERIFY(themeToolBar != nullptr);

    for (QToolBar *toolBar : {fileToolBar, scriptToolBar, workspaceToolBar, viewToolBar, selectionToolBar, themeToolBar}) {
        QVERIFY(toolBar != nullptr);
        QCOMPARE(toolBar->toolButtonStyle(), Qt::ToolButtonIconOnly);
        QVERIFY(toolBar->isMovable());
        QVERIFY(toolBar->isFloatable());
        QVERIFY(!toolBar->windowTitle().isEmpty());
    }

    QAction *runAction = nullptr;
    for (QAction *action : scriptToolBar->actions()) {
        if (action != nullptr && action->text() == QStringLiteral("Run Script")) {
            runAction = action;
            break;
        }
    }
    QVERIFY(runAction != nullptr);
    QVERIFY(!runAction->icon().isNull());

    QAction *lightThemeAction = nullptr;
    QAction *darkThemeAction = nullptr;
    for (QAction *action : themeToolBar->actions()) {
        if (action != nullptr && action->text() == QStringLiteral("Light")) {
            lightThemeAction = action;
        }
        if (action != nullptr && action->text() == QStringLiteral("Dark")) {
            darkThemeAction = action;
        }
    }
    QVERIFY(lightThemeAction != nullptr);
    QVERIFY(darkThemeAction != nullptr);
    QVERIFY(!lightThemeAction->icon().isNull());
    QVERIFY(!darkThemeAction->icon().isNull());
    QCOMPARE(lightThemeAction->toolTip(), QStringLiteral("Switch to light theme"));
    QCOMPARE(darkThemeAction->toolTip(), QStringLiteral("Switch to dark theme"));

    const QIcon themedIcon = pyraqt::ui::themedSvgIcon(QStringLiteral("sun"), QStringLiteral("dark"), QSize(20, 20));
    QVERIFY(!themedIcon.isNull());
    QVERIFY(!themedIcon.pixmap(QSize(20, 20), QIcon::Normal, QIcon::Off).isNull());
    QVERIFY(!themedIcon.pixmap(QSize(20, 20), QIcon::Disabled, QIcon::Off).isNull());

    auto *console = window.findChild<pyraqt::ui::PythonConsoleWidget *>();
    QVERIFY(console != nullptr);
    QVERIFY(console->actionButtonsHaveIcons());
    QVERIFY(!console->actionButtonsShowText());

    for (QToolBar *toolBar : {fileToolBar, scriptToolBar, workspaceToolBar, viewToolBar, selectionToolBar, themeToolBar}) {
        const QList<QToolButton *> toolButtons = toolBar->findChildren<QToolButton *>();
        QVERIFY(!toolButtons.isEmpty());
        for (QToolButton *button : toolButtons) {
            QVERIFY(button != nullptr);
            QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonIconOnly);
        }
    }

    QVERIFY(themeManager.setLightTheme());
    QVERIFY(!runAction->icon().isNull());
    QVERIFY(console->actionButtonsHaveIcons());

    QVERIFY(themeManager.setDarkTheme());
    QVERIFY(!runAction->icon().isNull());
    QVERIFY(console->actionButtonsHaveIcons());
}

void UiThemeTest::localeSwitchRefreshesOpenWidgetsAndCommands()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::I18nManager i18nManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    pyraqt::core::CrashRecoveryManager crashRecoveryManager(configManager, logManager);

    pyraqt::ui::MainWindow window(configManager,
        logManager,
        modelImportManager,
        themeManager,
        i18nManager,
        runtimeManager,
        executionManager,
        commandManager,
        pluginManager,
        workspaceManager,
        updateManager,
        crashRecoveryManager);
    window.show();
    QCoreApplication::processEvents();

    pyraqt::ui::SettingsDialog settingsDialog(configManager, themeManager, i18nManager, runtimeManager, updateManager, workspaceManager);
    settingsDialog.show();
    pyraqt::ui::CommandPaletteDialog paletteDialog(commandManager);
    paletteDialog.show();
    pyraqt::ui::PythonToolsDialog pythonToolsDialog(runtimeManager, executionManager);
    pythonToolsDialog.show();
    QCoreApplication::processEvents();

    QVERIFY(i18nManager.setLocale(QStringLiteral("zh_CN")) || i18nManager.currentLocale() == QStringLiteral("en_US"));
    QCoreApplication::processEvents();

    if (i18nManager.currentLocale() == QStringLiteral("zh_CN")) {
        QCOMPARE(window.findChild<QMenu *>(QStringLiteral("fileMenu"))->title(), QStringLiteral("文件(&F)"));
        QCOMPARE(settingsDialog.windowTitle(), QStringLiteral("设置"));
        QCOMPARE(paletteDialog.windowTitle(), QStringLiteral("命令面板"));
        QCOMPARE(pythonToolsDialog.windowTitle(), QStringLiteral("Python 工具"));

        bool hasChineseBuiltIn = false;
        for (const auto &command : commandManager.commands()) {
            if (command.ownerId == QStringLiteral("builtin") && command.source == QStringLiteral("内置")) {
                hasChineseBuiltIn = true;
                break;
            }
        }
        QVERIFY(hasChineseBuiltIn);
    }
}

void UiThemeTest::modelViewActionsUseStableViewKeys()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::I18nManager i18nManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    pyraqt::core::CrashRecoveryManager crashRecoveryManager(configManager, logManager);

    pyraqt::ui::MainWindow window(configManager,
        logManager,
        modelImportManager,
        themeManager,
        i18nManager,
        runtimeManager,
        executionManager,
        commandManager,
        pluginManager,
        workspaceManager,
        updateManager,
        crashRecoveryManager);

    auto *viewToolBar = window.findChild<QToolBar *>(QStringLiteral("viewToolBar"));
    QVERIFY(viewToolBar != nullptr);

    QAction *frontAction = nullptr;
    QAction *isoAction = nullptr;
    for (QAction *action : window.findChildren<QAction *>()) {
        if (action != nullptr && action->data().toString() == QStringLiteral("front")) {
            frontAction = action;
        }
        if (action != nullptr && action->data().toString() == QStringLiteral("isometric")) {
            isoAction = action;
        }
    }
    QVERIFY(frontAction != nullptr);
    QVERIFY(isoAction != nullptr);
    QCOMPARE(frontAction->data().toString(), QStringLiteral("front"));
    QCOMPARE(isoAction->data().toString(), QStringLiteral("isometric"));
}

void UiThemeTest::settingsDialogShowsResolvedInterpreterPath()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::ThemeManager themeManager(*qobject_cast<QApplication *>(QCoreApplication::instance()));
    pyraqt::core::I18nManager i18nManager(*qobject_cast<QApplication *>(QCoreApplication::instance()));
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);

    runtimeManager.setInterpreterPath(QStringLiteral("python3"));

    pyraqt::ui::SettingsDialog dialog(configManager, themeManager, i18nManager, runtimeManager, updateManager, workspaceManager);
    dialog.show();
    QCoreApplication::processEvents();

    const QList<QLineEdit *> edits = dialog.findChildren<QLineEdit *>();
    QLineEdit *interpreterEdit = nullptr;
    for (QLineEdit *edit : edits) {
        if (edit != nullptr && edit->toolTip().contains(QStringLiteral("python"))) {
            interpreterEdit = edit;
            break;
        }
    }
    QVERIFY(interpreterEdit != nullptr);

    const QString resolvedPython = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (!resolvedPython.isEmpty()) {
        QCOMPARE(interpreterEdit->text(), QFileInfo(resolvedPython).absoluteFilePath());
        QCOMPARE(interpreterEdit->toolTip(), QFileInfo(resolvedPython).absoluteFilePath());
    } else {
        QCOMPARE(interpreterEdit->text(), QStringLiteral("python3"));
    }
}

void UiThemeTest::mainWindowConsoleAndLogDocksCanBeCleared()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::I18nManager i18nManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    pyraqt::core::CrashRecoveryManager crashRecoveryManager(configManager, logManager);

    pyraqt::ui::MainWindow window(configManager,
        logManager,
        modelImportManager,
        themeManager,
        i18nManager,
        runtimeManager,
        executionManager,
        commandManager,
        pluginManager,
        workspaceManager,
        updateManager,
        crashRecoveryManager);
    window.show();
    QCoreApplication::processEvents();

    auto *console = window.findChild<pyraqt::ui::PythonConsoleWidget *>();
    QVERIFY(console != nullptr);
    console->appendOutput(QStringLiteral("stdout"), QStringLiteral("console line"));

    auto *logViewerDock = window.findChild<QDockWidget *>(QStringLiteral("logViewerDock"));
    QVERIFY(logViewerDock != nullptr);
    auto *logViewer = logViewerDock->findChild<QPlainTextEdit *>();
    QVERIFY(logViewer != nullptr);
    logViewer->appendPlainText(QStringLiteral("log line"));

    const QList<QToolButton *> clearButtons = window.findChildren<QToolButton *>();
    QToolButton *clearConsoleButton = nullptr;
    QToolButton *clearLogButton = nullptr;
    for (QToolButton *button : clearButtons) {
        if (button->toolTip() == QStringLiteral("Clear console output")) {
            clearConsoleButton = button;
        } else if (button->toolTip() == QStringLiteral("Clear log output")) {
            clearLogButton = button;
        }
    }
    QVERIFY(clearConsoleButton != nullptr);
    QVERIFY(clearLogButton != nullptr);

    clearConsoleButton->click();
    clearLogButton->click();
    QCoreApplication::processEvents();

    QVERIFY(console->outputTextForTesting().isEmpty());
    QVERIFY(logViewer->toPlainText().isEmpty());
}

void UiThemeTest::mainWindowPropertiesDockTracksCurrentDocumentKind()
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString textPath = dir.filePath(QStringLiteral("notes.txt"));
    QFile textFile(textPath);
    QVERIFY(textFile.open(QIODevice::WriteOnly | QIODevice::Text));
    textFile.write("plain text\n");
    textFile.close();

    const QString modelPath = dir.filePath(QStringLiteral("shape.brep"));
    QFile modelFile(modelPath);
    QVERIFY(modelFile.open(QIODevice::WriteOnly | QIODevice::Text));
    modelFile.write("BREP placeholder\n");
    modelFile.close();

    const QString binaryPath = dir.filePath(QStringLiteral("archive.bin"));
    QFile binaryFile(binaryPath);
    QVERIFY(binaryFile.open(QIODevice::WriteOnly));
    binaryFile.write(QByteArray("abc\0def", 7));
    binaryFile.close();

    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::ThemeManager themeManager(*app);
    pyraqt::core::I18nManager i18nManager(*app);
    pyraqt::core::ModelImportManager modelImportManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);
    pyraqt::core::WorkspaceManager workspaceManager(configManager);
    pyraqt::core::UpdateManager updateManager(configManager, logManager);
    pyraqt::core::CrashRecoveryManager crashRecoveryManager(configManager, logManager);

    pyraqt::ui::MainWindow window(configManager,
        logManager,
        modelImportManager,
        themeManager,
        i18nManager,
        runtimeManager,
        executionManager,
        commandManager,
        pluginManager,
        workspaceManager,
        updateManager,
        crashRecoveryManager);

    auto *propertiesDock = window.findChild<QDockWidget *>(QStringLiteral("propertiesDock"));
    auto *workspace = window.findChild<pyraqt::ui::EditorWorkspaceWidget *>(QStringLiteral("editorWorkspace"));
    QVERIFY(propertiesDock != nullptr);
    QVERIFY(workspace != nullptr);
    window.show();
    QCoreApplication::processEvents();
    QVERIFY(!propertiesDock->isVisible());

    QVERIFY(workspace->openPath(textPath));
    QCoreApplication::processEvents();
    QVERIFY(!propertiesDock->isVisible());

    QTimer::singleShot(0, [] {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *messageBox = qobject_cast<QMessageBox *>(widget)) {
                messageBox->accept();
            }
        }
    });
    QVERIFY(workspace->openPath(modelPath));
    QCoreApplication::processEvents();
    QVERIFY(propertiesDock->isVisible());

    QVERIFY(workspace->openPath(textPath));
    QCoreApplication::processEvents();
    QVERIFY(!propertiesDock->isVisible());

    QVERIFY(workspace->openPath(binaryPath));
    QCoreApplication::processEvents();
    QVERIFY(!propertiesDock->isVisible());
}

void UiThemeTest::modelPropertiesPanelUsesFixedMinimumWidthAndTooltips()
{
    pyraqt::ui::ModelPropertiesPanel panel;
    panel.resize(280, 320);
    panel.show();
    QCoreApplication::processEvents();

    QCOMPARE(panel.minimumWidth(), 280);

    pyraqt::core::ModelDocument document;
    document.statusMessage = QStringLiteral("Loaded");
    document.filePath = QStringLiteral("/tmp/projects/a/very/long/path/to/a-model-with-a-long-name.brep");
    document.boundingBoxText = QStringLiteral("X:[-1000.0, 1000.0] Y:[-1000.0, 1000.0] Z:[-1000.0, 1000.0]");
    panel.setModelDocument(document);
    QCoreApplication::processEvents();

    auto *fileLabel = panel.findChild<QLabel *>(QStringLiteral("propertiesFileValue"));
    QVERIFY(fileLabel != nullptr);
    QCOMPARE(fileLabel->toolTip(), document.filePath);
    QCOMPARE(fileLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    QVERIFY(fileLabel->text().contains(QStringLiteral("...")) || fileLabel->text().contains(QChar(0x2026)));
    QVERIFY(fileLabel->text() != document.filePath);
}

void UiThemeTest::fileBrowserPanelProvidesTooltips()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString nestedDir = dir.filePath(QStringLiteral("nested"));
    QDir().mkpath(nestedDir);
    const QString filePath = QDir(nestedDir).filePath(QStringLiteral("very_long_filename_for_tooltip_check.txt"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("hello\n");
    file.close();

    pyraqt::ui::FileBrowserPanel panel;
    panel.setRootPath(dir.path());

    auto *treeView = panel.findChild<QTreeView *>();
    QVERIFY(treeView != nullptr);

    const auto *model = qobject_cast<const QFileSystemModel *>(treeView->model());
    QVERIFY(model != nullptr);
    const QModelIndex fileIndex = model->index(filePath);
    QVERIFY(fileIndex.isValid());
    QCOMPARE(panel.toolTipForIndex(fileIndex), filePath);
    QCOMPARE(fileIndex.data(Qt::ToolTipRole).toString(), filePath);
}

void UiThemeTest::pluginManagerPanelProvidesTooltips()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

    pluginManager.scanPlugins();

    pyraqt::ui::PluginManagerPanel panel(pluginManager);
    QTableWidget *table = panel.tableForTesting();
    QVERIFY(table != nullptr);
    QVERIFY(table->rowCount() > 0);

    QTableWidgetItem *nameItem = table->item(0, 1);
    QTableWidgetItem *statusItem = table->item(0, 4);
    QTableWidgetItem *detailsItem = table->item(0, 6);
    QVERIFY(nameItem != nullptr);
    QVERIFY(statusItem != nullptr);
    QVERIFY(detailsItem != nullptr);
    QCOMPARE(nameItem->toolTip(), nameItem->text());
    QCOMPARE(statusItem->toolTip(), statusItem->text());
    QVERIFY(!detailsItem->toolTip().isNull());
}

void UiThemeTest::themedFileDialogUsesQtDialogSettings()
{
    const pyraqt::ui::FileDialogRequest request{
        QStringLiteral("Open File"),
        QStringLiteral("/tmp"),
        QStringLiteral("Python Files (*.py);;All Files (*)"),
        QStringLiteral("py"),
        QFileDialog::ExistingFile,
        QFileDialog::AcceptOpen,
    };

    std::unique_ptr<QFileDialog> dialog(pyraqt::ui::createThemedFileDialog(request));
    QVERIFY(dialog->testOption(QFileDialog::DontUseNativeDialog));
    QCOMPARE(dialog->fileMode(), QFileDialog::ExistingFile);
    QCOMPARE(dialog->acceptMode(), QFileDialog::AcceptOpen);
    QCOMPARE(dialog->directory().absolutePath(), QStringLiteral("/tmp"));
    QCOMPARE(dialog->defaultSuffix(), QStringLiteral("py"));
    const QStringList expectedFilters{QStringLiteral("Python Files (*.py)"), QStringLiteral("All Files (*)")};
    QCOMPARE(dialog->nameFilters(), expectedFilters);
}

void UiThemeTest::themedFileDialogUsesRequestedDefaultSuffix()
{
    const pyraqt::ui::FileDialogRequest request{
        QStringLiteral("Save File"),
        QStringLiteral("/tmp/example.stp"),
        QStringLiteral("STP Files (*.stp);;All Files (*)"),
        QStringLiteral("stp"),
        QFileDialog::AnyFile,
        QFileDialog::AcceptSave,
    };

    std::unique_ptr<QFileDialog> dialog(pyraqt::ui::createThemedFileDialog(request));
    QCOMPARE(dialog->defaultSuffix(), QStringLiteral("stp"));
    const QStringList expectedFilters{QStringLiteral("STP Files (*.stp)"), QStringLiteral("All Files (*)")};
    QCOMPARE(dialog->nameFilters(), expectedFilters);
}

} // namespace

QObject *createUiThemeTest()
{
    return new UiThemeTest();
}

#include "test_ui_theme.moc"
