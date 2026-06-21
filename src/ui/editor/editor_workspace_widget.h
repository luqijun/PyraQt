#pragma once

#include "core/workspace/workspace_types.h"

#include <QWidget>

class QTabWidget;

namespace pyraqt::core {
class ModelImportManager;
struct ModelDocument;
struct ModelSelectionInfo;
class PythonRuntimeManager;
class ThemeManager;
} // namespace pyraqt::core

namespace pyraqt::ui {

class EditorPlaceholderWidget;
class ModelDocumentWidget;
class ScriptEditorWidget;

class EditorWorkspaceWidget final : public QWidget {
    Q_OBJECT

public:
    enum class DocumentKind {
        Text,
        Model,
        PreviewUnavailable,
        None,
    };

    explicit EditorWorkspaceWidget(
        core::ThemeManager &themeManager,
        core::ModelImportManager &modelImportManager,
        core::PythonRuntimeManager *pythonRuntimeManager = nullptr,
        QWidget *parent = nullptr);

    ScriptEditorWidget *newDocument();
    bool openPath(const QString &filePath);
    bool saveCurrent();
    bool saveAll();
    bool saveCurrentAs(const QString &filePath);
    bool hasOpenPath(const QString &filePath) const;
    bool renameOpenPath(const QString &oldPath, const QString &newPath);
    bool closePath(const QString &filePath);
    bool closeCurrent();
    bool closeEditorAt(int index);
    void closeOtherEditors();
    void closeEditorsToRight();
    bool closeAllEditors();

    [[nodiscard]] ScriptEditorWidget *currentEditor() const;
    [[nodiscard]] ScriptEditorWidget *editorAt(int index) const;
    [[nodiscard]] ModelDocumentWidget *currentModelDocumentWidget() const;
    [[nodiscard]] pyraqt::core::ModelDocument currentModelDocument() const;
    [[nodiscard]] pyraqt::core::ModelSelectionInfo currentModelSelection() const;
    [[nodiscard]] QString currentFilePath() const;
    [[nodiscard]] DocumentKind currentDocumentKind() const;
    [[nodiscard]] bool currentFileRunnable() const;
    [[nodiscard]] bool hasOpenEditors() const;
    [[nodiscard]] bool hasAvailableEditor() const;
    [[nodiscard]] bool hasEditorsToRight() const;
    [[nodiscard]] int editorCount() const;
    [[nodiscard]] QStringList openFilePaths() const;
    [[nodiscard]] core::WorkspaceSession captureSession(const QStringList &recentFiles, const QString &fileBrowserRoot) const;
    void restoreSession(const core::WorkspaceSession &session);

signals:
    void currentFilePathChanged(const QString &filePath);
    void currentCursorChanged(int line, int column);
    void documentModificationChanged(bool modified);
    void editorAvailabilityChanged(bool available);
    void modelDocumentChanged(const pyraqt::core::ModelDocument &document);
    void modelSelectionChanged(const pyraqt::core::ModelSelectionInfo &selection);
    void openFilesChanged(const QStringList &files);
    void requestCloseConfirmation(const QString &title, const QString &message, bool *accepted);
    void openPathFailed(const QString &filePath, const QString &message);

private:
    ScriptEditorWidget *createEditor();
    ModelDocumentWidget *createModelDocumentWidget(const pyraqt::core::ModelDocument &document);
    EditorPlaceholderWidget *createPreviewUnavailableWidget(const QString &filePath);
    void updateTabTitle(int index);
    void connectEditor(ScriptEditorWidget *editor);
    void connectModelDocumentWidget(ModelDocumentWidget *widget);
    [[nodiscard]] QString filePathForWidget(QWidget *widget) const;
    [[nodiscard]] DocumentKind documentKindForWidget(QWidget *widget) const;
    [[nodiscard]] bool isEditableTextFile(const QString &filePath) const;
    void emitCurrentWidgetState();
    void showTabContextMenu(const QPoint &position);
    int findEditorByPath(const QString &filePath) const;
    int findWidgetByPath(const QString &filePath) const;
    bool closeOtherEditors(int keepIndex);
    bool closeEditorsToRight(int index);
    bool closeEditorInternal(int index);

    core::ThemeManager &m_themeManager;
    core::ModelImportManager &m_modelImportManager;
    core::PythonRuntimeManager *m_pythonRuntimeManager = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    int m_untitledCounter = 1;
};

} // namespace pyraqt::ui
