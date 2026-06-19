#pragma once

#include "core/workspace/workspace_types.h"

#include <QWidget>

class QTabWidget;

namespace pyraqt::ui {

class ScriptEditorWidget;

class EditorWorkspaceWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EditorWorkspaceWidget(QWidget *parent = nullptr);

    ScriptEditorWidget *newDocument();
    bool openFile(const QString &filePath);
    bool saveCurrent();
    bool saveAll();
    bool saveCurrentAs(const QString &filePath);
    bool closeCurrent();
    bool closeEditorAt(int index);
    void closeOtherEditors();
    bool closeAllEditors();

    [[nodiscard]] ScriptEditorWidget *currentEditor() const;
    [[nodiscard]] ScriptEditorWidget *editorAt(int index) const;
    [[nodiscard]] QString currentFilePath() const;
    [[nodiscard]] bool hasOpenEditors() const;
    [[nodiscard]] bool hasAvailableEditor() const;
    [[nodiscard]] int editorCount() const;
    [[nodiscard]] QStringList openFilePaths() const;
    [[nodiscard]] core::WorkspaceSession captureSession(const QStringList &recentFiles, const QString &fileBrowserRoot) const;
    void restoreSession(const core::WorkspaceSession &session);

signals:
    void currentFilePathChanged(const QString &filePath);
    void currentCursorChanged(int line, int column);
    void documentModificationChanged(bool modified);
    void editorAvailabilityChanged(bool available);
    void openFilesChanged(const QStringList &files);
    void requestCloseConfirmation(const QString &title, const QString &message, bool *accepted);

private:
    ScriptEditorWidget *createEditor();
    void updateTabTitle(int index);
    void connectEditor(ScriptEditorWidget *editor);
    int findEditorByPath(const QString &filePath) const;
    bool closeEditorInternal(int index);

    QTabWidget *m_tabWidget = nullptr;
    int m_untitledCounter = 1;
};

} // namespace pyraqt::ui
