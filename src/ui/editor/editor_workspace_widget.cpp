#include "ui/editor/editor_workspace_widget.h"

#include "core/modeling/model_import_manager.h"
#include "core/modeling/model_types.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/theme/theme_manager.h"
#include "ui/editor/model_document_widget.h"
#include "ui/editor/script_editor_widget.h"

#include <QFileInfo>
#include <QTabWidget>
#include <QVBoxLayout>

namespace pyraqt::ui {

EditorWorkspaceWidget::EditorWorkspaceWidget(
    core::ThemeManager &themeManager,
    core::ModelImportManager &modelImportManager,
    core::PythonRuntimeManager *pythonRuntimeManager,
    QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_modelImportManager(modelImportManager)
    , m_pythonRuntimeManager(pythonRuntimeManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setAccessibleName(tr("Document Workspace"));
    m_tabWidget->setAccessibleDescription(tr("Tabbed workspace for scripts and imported models"));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        emitCurrentWidgetState();
        emit openFilesChanged(openFilePaths());
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

bool EditorWorkspaceWidget::openPath(const QString &filePath)
{
    const int existingIndex = findWidgetByPath(filePath);
    if (existingIndex >= 0) {
        m_tabWidget->setCurrentIndex(existingIndex);
        return true;
    }

    if (m_modelImportManager.isSupportedFile(filePath)) {
        const core::ModelDocument document = m_modelImportManager.importFile(filePath);
        if (!document.isValid) {
            emit openPathFailed(filePath, document.summary.errorMessage);
        }
        ModelDocumentWidget *documentWidget = createModelDocumentWidget(document);
        const int index = m_tabWidget->addTab(documentWidget, QFileInfo(filePath).fileName());
        m_tabWidget->setCurrentIndex(index);
        updateTabTitle(index);
        emit openFilesChanged(openFilePaths());
        return true;
    }

    ScriptEditorWidget *editor = createEditor();
    if (!editor->loadFromFile(filePath)) {
        editor->deleteLater();
        emit openPathFailed(filePath, tr("Failed to open file."));
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

bool EditorWorkspaceWidget::hasOpenPath(const QString &filePath) const
{
    return findWidgetByPath(filePath) >= 0;
}

bool EditorWorkspaceWidget::renameOpenPath(const QString &oldPath, const QString &newPath)
{
    const int index = findWidgetByPath(oldPath);
    if (index < 0) {
        return false;
    }

    QWidget *widget = m_tabWidget->widget(index);
    if (auto *editor = qobject_cast<ScriptEditorWidget *>(widget)) {
        editor->setCurrentFilePath(newPath);
    } else if (auto *documentWidget = qobject_cast<ModelDocumentWidget *>(widget)) {
        documentWidget->setDocumentFilePath(newPath);
        updateTabTitle(index);
        if (widget == m_tabWidget->currentWidget()) {
            emitCurrentWidgetState();
        }
        emit openFilesChanged(openFilePaths());
    } else {
        return false;
    }

    updateTabTitle(index);
    if (widget == m_tabWidget->currentWidget()) {
        emitCurrentWidgetState();
    }
    emit openFilesChanged(openFilePaths());
    return true;
}

bool EditorWorkspaceWidget::closePath(const QString &filePath)
{
    const int index = findWidgetByPath(filePath);
    return index >= 0 ? closeEditorInternal(index) : false;
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

ModelDocumentWidget *EditorWorkspaceWidget::currentModelDocumentWidget() const
{
    return qobject_cast<ModelDocumentWidget *>(m_tabWidget->currentWidget());
}

pyraqt::core::ModelDocument EditorWorkspaceWidget::currentModelDocument() const
{
    if (auto *documentWidget = currentModelDocumentWidget()) {
        return documentWidget->document();
    }
    return {};
}

pyraqt::core::ModelSelectionInfo EditorWorkspaceWidget::currentModelSelection() const
{
    if (auto *documentWidget = currentModelDocumentWidget()) {
        return documentWidget->selectionInfo();
    }
    return {};
}

QString EditorWorkspaceWidget::currentFilePath() const
{
    QWidget *widget = m_tabWidget->currentWidget();
    if (auto *editor = qobject_cast<ScriptEditorWidget *>(widget)) {
        return editor->currentFilePath();
    }
    if (auto *documentWidget = qobject_cast<ModelDocumentWidget *>(widget)) {
        return documentWidget->document().filePath;
    }
    return {};
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
        QWidget *widget = m_tabWidget->widget(index);
        QString filePath;
        if (auto *editor = qobject_cast<ScriptEditorWidget *>(widget)) {
            filePath = editor->currentFilePath();
        } else if (auto *documentWidget = qobject_cast<ModelDocumentWidget *>(widget)) {
            filePath = documentWidget->document().filePath;
        }

        if (!filePath.isEmpty()) {
            files.push_back(filePath);
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
        openPath(filePath);
    }

    if (!session.activeFile.isEmpty()) {
        const int activeIndex = findWidgetByPath(session.activeFile);
        if (activeIndex >= 0) {
            m_tabWidget->setCurrentIndex(activeIndex);
        }
    }
}

ScriptEditorWidget *EditorWorkspaceWidget::createEditor()
{
    auto *editor = new ScriptEditorWidget(m_pythonRuntimeManager, this);
    editor->applyTheme(m_themeManager.currentTheme());
    connectEditor(editor);
    return editor;
}

ModelDocumentWidget *EditorWorkspaceWidget::createModelDocumentWidget(const pyraqt::core::ModelDocument &document)
{
    auto *widget = new ModelDocumentWidget(this);
    widget->setDocument(document);
    connectModelDocumentWidget(widget);
    return widget;
}

void EditorWorkspaceWidget::updateTabTitle(int index)
{
    QWidget *widget = m_tabWidget->widget(index);
    if (widget == nullptr) {
        return;
    }

    QString title;
    QString toolTip;

    if (auto *editor = qobject_cast<ScriptEditorWidget *>(widget)) {
        title = QFileInfo(editor->currentFilePath()).fileName();
        if (title.isEmpty()) {
            title = m_tabWidget->tabText(index);
            if (title.isEmpty()) {
                title = tr("Untitled");
            }
        }
        if (editor->isModified()) {
            title.append('*');
        }
        toolTip = editor->currentFilePath();
    } else if (auto *documentWidget = qobject_cast<ModelDocumentWidget *>(widget)) {
        toolTip = documentWidget->document().filePath;
        title = QFileInfo(toolTip).fileName();
        if (title.isEmpty()) {
            title = tr("Model");
        }
    }

    m_tabWidget->setTabText(index, title);
    m_tabWidget->setTabToolTip(index, toolTip);
}

void EditorWorkspaceWidget::connectModelDocumentWidget(ModelDocumentWidget *widget)
{
    connect(widget, &ModelDocumentWidget::displayModeChanged, this, [this](pyraqt::core::ModelDisplayMode) {
        emitCurrentWidgetState();
    });
    connect(widget, &ModelDocumentWidget::selectionModeChanged, this, [this](pyraqt::core::ModelSelectionMode) {
        emitCurrentWidgetState();
    });
    connect(widget, &ModelDocumentWidget::selectionInfoChanged, this, [this](const pyraqt::core::ModelSelectionInfo &selection) {
        emit modelSelectionChanged(selection);
    });
    connect(widget, &ModelDocumentWidget::statusMessageChanged, this, [this](const QString &) {
        emitCurrentWidgetState();
    });
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

void EditorWorkspaceWidget::emitCurrentWidgetState()
{
    QWidget *widget = m_tabWidget->currentWidget();
    if (auto *editor = qobject_cast<ScriptEditorWidget *>(widget)) {
        emit currentFilePathChanged(editor->currentFilePath());
        emit documentModificationChanged(editor->isModified());
        emit editorAvailabilityChanged(editor->isAvailable());
        emit currentCursorChanged(editor->currentLine(), editor->currentColumn());
        emit modelDocumentChanged({});
        emit modelSelectionChanged({});
        return;
    }

    if (auto *documentWidget = qobject_cast<ModelDocumentWidget *>(widget)) {
        emit currentFilePathChanged(documentWidget->document().filePath);
        emit documentModificationChanged(false);
        emit editorAvailabilityChanged(false);
        emit currentCursorChanged(0, 0);
        emit modelDocumentChanged(documentWidget->document());
        emit modelSelectionChanged(documentWidget->selectionInfo());
        return;
    }

    emit currentFilePathChanged({});
    emit documentModificationChanged(false);
    emit editorAvailabilityChanged(false);
    emit currentCursorChanged(0, 0);
    emit modelDocumentChanged({});
    emit modelSelectionChanged({});
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

int EditorWorkspaceWidget::findWidgetByPath(const QString &filePath) const
{
    const int editorIndex = findEditorByPath(filePath);
    if (editorIndex >= 0) {
        return editorIndex;
    }

    for (int index = 0; index < m_tabWidget->count(); ++index) {
        auto *documentWidget = qobject_cast<ModelDocumentWidget *>(m_tabWidget->widget(index));
        if (documentWidget != nullptr && documentWidget->document().filePath == filePath) {
            return index;
        }
    }
    return -1;
}

bool EditorWorkspaceWidget::closeEditorInternal(int index)
{
    auto *editor = qobject_cast<ScriptEditorWidget *>(m_tabWidget->widget(index));
    if (editor != nullptr && editor->isModified()) {
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
    if (widget == nullptr) {
        return true;
    }

    m_tabWidget->removeTab(index);
    widget->deleteLater();
    emit openFilesChanged(openFilePaths());
    if (m_tabWidget->count() == 0) {
        newDocument();
    } else {
        emitCurrentWidgetState();
    }
    return true;
}

} // namespace pyraqt::ui
