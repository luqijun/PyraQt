#include "ui/editor/editor_workspace_widget.h"

#include "core/theme/theme_manager.h"
#include "ui/editor/script_editor_widget.h"

#include <QFileInfo>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace pyraqt::ui {

EditorWorkspaceWidget::EditorWorkspaceWidget(core::ThemeManager &themeManager, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setAccessibleName(tr("Editor Workspace"));
    m_tabWidget->setAccessibleDescription(tr("Tabbed script editor workspace"));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        ScriptEditorWidget *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
        emit currentFilePathChanged(editor != nullptr ? editor->currentFilePath() : QString());
        emit documentModificationChanged(editor != nullptr && editor->isModified());
        emit editorAvailabilityChanged(editor != nullptr && editor->isAvailable());
        emit openFilesChanged(openFilePaths());
        if (editor != nullptr) {
            emit currentCursorChanged(editor->currentLine(), editor->currentColumn());
        }
    });
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeEditorInternal(index);
    });
    connect(&m_themeManager, &core::ThemeManager::themeChanged, this, [this](const QString &themeName) {
        for (int index = 0; index < m_tabWidget->count(); ++index) {
            if (auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index))) {
                editor->applyTheme(themeName);
            }
        }
    });
}

ScriptEditorWidget *EditorWorkspaceWidget::newDocument()
{
    ScriptEditorWidget *editor = createEditor();
    editor->newDocument();
    const int index = m_tabWidget->addTab(editor, tr("Untitled %1").arg(m_untitledCounter++));
    m_tabWidget->setCurrentIndex(index);
    emit openFilesChanged(openFilePaths());
    return editor;
}

bool EditorWorkspaceWidget::openFile(const QString &filePath)
{
    const int existingIndex = findEditorByPath(filePath);
    if (existingIndex >= 0) {
        m_tabWidget->setCurrentIndex(existingIndex);
        return true;
    }

    ScriptEditorWidget *editor = createEditor();
    if (!editor->loadFromFile(filePath)) {
        editor->deleteLater();
        return false;
    }

    const int index = m_tabWidget->addTab(editor, QFileInfo(filePath).fileName());
    m_tabWidget->setCurrentIndex(index);
    updateTabTitle(index);
    emit openFilesChanged(openFilePaths());
    return true;
}

bool EditorWorkspaceWidget::saveCurrent()
{
    ScriptEditorWidget *editor = currentEditor();
    return editor != nullptr && editor->save();
}

bool EditorWorkspaceWidget::saveAll()
{
    for (int index = 0; index < m_tabWidget->count(); ++index) {
        auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
        if (editor == nullptr || editor->currentFilePath().isEmpty()) {
            continue;
        }
        if (!editor->save()) {
            return false;
        }
    }
    return true;
}

bool EditorWorkspaceWidget::saveCurrentAs(const QString &filePath)
{
    ScriptEditorWidget *editor = currentEditor();
    if (editor == nullptr) {
        return false;
    }
    const bool saved = editor->saveAs(filePath);
    if (saved) {
        updateTabTitle(m_tabWidget->currentIndex());
        emit openFilesChanged(openFilePaths());
    }
    return saved;
}

bool EditorWorkspaceWidget::closeCurrent()
{
    return closeEditorInternal(m_tabWidget->currentIndex());
}

bool EditorWorkspaceWidget::closeEditorAt(int index)
{
    return closeEditorInternal(index);
}

void EditorWorkspaceWidget::closeOtherEditors()
{
    const int currentIndex = m_tabWidget->currentIndex();
    for (int index = m_tabWidget->count() - 1; index >= 0; --index) {
        if (index != currentIndex) {
            closeEditorInternal(index);
        }
    }
}

bool EditorWorkspaceWidget::closeAllEditors()
{
    for (int index = m_tabWidget->count() - 1; index >= 0; --index) {
        if (!closeEditorInternal(index)) {
            return false;
        }
    }
    return true;
}

ScriptEditorWidget *EditorWorkspaceWidget::currentEditor() const
{
    return qobject_cast<ScriptEditorWidget *>(m_tabWidget->currentWidget());
}

ScriptEditorWidget *EditorWorkspaceWidget::editorAt(int index) const
{
    return qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
}

QString EditorWorkspaceWidget::currentFilePath() const
{
    ScriptEditorWidget *editor = currentEditor();
    return editor != nullptr ? editor->currentFilePath() : QString();
}

bool EditorWorkspaceWidget::hasOpenEditors() const
{
    return m_tabWidget->count() > 0;
}

bool EditorWorkspaceWidget::hasAvailableEditor() const
{
    ScriptEditorWidget *editor = currentEditor();
    return editor != nullptr && editor->isAvailable();
}

int EditorWorkspaceWidget::editorCount() const
{
    return m_tabWidget->count();
}

QStringList EditorWorkspaceWidget::openFilePaths() const
{
    QStringList files;
    for (int index = 0; index < m_tabWidget->count(); ++index) {
        auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
        if (editor != nullptr && !editor->currentFilePath().isEmpty()) {
            files.push_back(editor->currentFilePath());
        }
    }
    return files;
}

core::WorkspaceSession EditorWorkspaceWidget::captureSession(const QStringList &recentFiles, const QString &fileBrowserRoot) const
{
    core::WorkspaceSession session;
    session.openFiles = openFilePaths();
    session.activeFile = currentFilePath();
    session.recentFiles = recentFiles;
    session.fileBrowserRoot = fileBrowserRoot;
    return session;
}

void EditorWorkspaceWidget::restoreSession(const core::WorkspaceSession &session)
{
    if (session.openFiles.isEmpty()) {
        newDocument();
        return;
    }

    for (const QString &filePath : session.openFiles) {
        openFile(filePath);
    }

    if (!session.activeFile.isEmpty()) {
        const int activeIndex = findEditorByPath(session.activeFile);
        if (activeIndex >= 0) {
            m_tabWidget->setCurrentIndex(activeIndex);
        }
    }
}

ScriptEditorWidget *EditorWorkspaceWidget::createEditor()
{
    auto *editor = new ScriptEditorWidget(this);
    editor->applyTheme(m_themeManager.currentTheme());
    connectEditor(editor);
    return editor;
}

void EditorWorkspaceWidget::updateTabTitle(int index)
{
    auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
    if (editor == nullptr) {
        return;
    }
    QString title = QFileInfo(editor->currentFilePath()).fileName();
    if (title.isEmpty()) {
        title = m_tabWidget->tabText(index);
        if (title.isEmpty()) {
            title = tr("Untitled");
        }
    }
    if (editor->isModified()) {
        title.append('*');
    }
    m_tabWidget->setTabText(index, title);
    m_tabWidget->setTabToolTip(index, editor->currentFilePath());
}

void EditorWorkspaceWidget::connectEditor(ScriptEditorWidget *editor)
{
    connect(editor, &ScriptEditorWidget::modificationChanged, this, [this, editor](bool modified) {
        const int index = m_tabWidget->indexOf(editor);
        if (index >= 0) {
            updateTabTitle(index);
        }
        if (editor == currentEditor()) {
            emit documentModificationChanged(modified);
        }
    });
    connect(editor, &ScriptEditorWidget::filePathChanged, this, [this, editor](const QString &) {
        const int index = m_tabWidget->indexOf(editor);
        if (index >= 0) {
            updateTabTitle(index);
        }
        if (editor == currentEditor()) {
            emit currentFilePathChanged(editor->currentFilePath());
        }
        emit openFilesChanged(openFilePaths());
    });
    connect(editor, &ScriptEditorWidget::cursorPositionChanged, this, [this, editor](int line, int column) {
        if (editor == currentEditor()) {
            emit currentCursorChanged(line, column);
        }
    });
}

int EditorWorkspaceWidget::findEditorByPath(const QString &filePath) const
{
    for (int index = 0; index < m_tabWidget->count(); ++index) {
        auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
        if (editor != nullptr && editor->currentFilePath() == filePath) {
            return index;
        }
    }
    return -1;
}

bool EditorWorkspaceWidget::closeEditorInternal(int index)
{
    auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
    if (editor == nullptr) {
        return true;
    }

    if (editor->isModified()) {
        bool accepted = false;
        emit requestCloseConfirmation(
            tr("Close Script"),
            tr("The current script has unsaved changes. Close it anyway?"),
            &accepted);
        if (!accepted) {
            return false;
        }
    }

    QWidget *widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    widget->deleteLater();
    emit openFilesChanged(openFilePaths());
    if (m_tabWidget->count() == 0) {
        newDocument();
    }
    return true;
}

} // namespace pyraqt::ui
