#include "ui/editor/script_editor_widget.h"

#include "core/scripting/python/python_completion_provider.h"
#include "core/scripting/python/python_runtime_manager.h"

#include <QColor>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QPalette>
#include <QTextEdit>
#include <QVBoxLayout>

#if PYRAQT_HAS_QSCINTILLA
#include <Qsci/qsciapis.h>
#include <Qsci/qsciscintillabase.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qsciscintilla.h>
#endif

namespace pyraqt::ui {

ScriptEditorWidget::ScriptEditorWidget(core::PythonRuntimeManager *runtimeManager, QWidget *parent)
    : QWidget(parent)
    , m_runtimeManager(runtimeManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

#if PYRAQT_HAS_QSCINTILLA
    m_editor = new QsciScintilla(this);
    m_lexer = new QsciLexerPython(this);
    m_editor->setUtf8(true);
    m_editor->setIndentationWidth(4);
    m_editor->setAutoIndent(true);
    m_editor->setMarginLineNumbers(1, true);
    m_editor->setMarginWidth(1, QStringLiteral("00000"));
    m_editor->setBraceMatching(QsciScintilla::StrictBraceMatch);
    m_editor->setCaretLineVisible(true);
    m_editor->setCaretLineBackgroundColor(QColor(QStringLiteral("#bccbee")));
    m_editor->installEventFilter(this);
    configureCodeCompletion();
    m_editor->setText(QString());
    connect(m_editor, &QsciScintilla::textChanged, this, [this] {
        setModified(true);
    });
    connect(m_editor, &QsciScintilla::cursorPositionChanged, this, [this](int line, int column) {
        emit cursorPositionChanged(line + 1, column + 1);
    });
    connect(m_editor, &QsciScintilla::userListActivated, this, [this](int id, const QString &text) {
        if (id == 1 && m_editor != nullptr) {
            int line = 0;
            int column = 0;
            m_editor->getCursorPosition(&line, &column);
            const QString prefix = core::PythonCompletionProvider::prefixBeforeCursor(
                m_editor->text(),
                m_editor->positionFromLineIndex(line, column));
            const QString memberPrefix = core::PythonCompletionProvider::memberNamePrefixForDottedPrefix(prefix);
            if (!memberPrefix.isEmpty()) {
                m_editor->setSelection(line, column - memberPrefix.length(), line, column);
                m_editor->replaceSelectedText(text);
            } else {
                m_editor->insert(text);
            }
        }
    });
    layout->addWidget(m_editor);
#else
    m_placeholder = new QLabel(this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);
    layout->addWidget(m_placeholder);
#endif

    setDocumentMode(DocumentMode::PlainText);
    applyTheme(m_appliedTheme);
    retranslateUi();
}

ScriptEditorWidget::~ScriptEditorWidget()
{
#if PYRAQT_HAS_QSCINTILLA
    delete m_apis;
    m_apis = nullptr;
#endif
}

bool ScriptEditorWidget::isAvailable() const
{
#if PYRAQT_HAS_QSCINTILLA
    return true;
#else
    return false;
#endif
}

QString ScriptEditorWidget::currentFilePath() const
{
    return m_currentFilePath;
}

QString ScriptEditorWidget::currentText() const
{
#if PYRAQT_HAS_QSCINTILLA
    return m_editor->text();
#else
    return {};
#endif
}

QString ScriptEditorWidget::selectedText() const
{
#if PYRAQT_HAS_QSCINTILLA
    return m_editor->selectedText();
#else
    return {};
#endif
}

bool ScriptEditorWidget::isModified() const
{
    return m_modified;
}

int ScriptEditorWidget::currentLine() const
{
#if PYRAQT_HAS_QSCINTILLA
    int line = 0;
    int column = 0;
    m_editor->getCursorPosition(&line, &column);
    return line + 1;
#else
    return 1;
#endif
}

int ScriptEditorWidget::currentColumn() const
{
#if PYRAQT_HAS_QSCINTILLA
    int line = 0;
    int column = 0;
    m_editor->getCursorPosition(&line, &column);
    Q_UNUSED(line)
    return column + 1;
#else
    return 1;
#endif
}

QString ScriptEditorWidget::appliedTheme() const
{
    return m_appliedTheme;
}

ScriptEditorWidget::DocumentMode ScriptEditorWidget::documentMode() const
{
    return m_documentMode;
}

bool ScriptEditorWidget::isPythonDocument() const
{
    return m_documentMode == DocumentMode::Python;
}

bool ScriptEditorWidget::codeCompletionEnabled() const
{
    return m_codeCompletionEnabled;
}

bool ScriptEditorWidget::dotCompletionEnabled() const
{
    return m_dotCompletionEnabled;
}

QStringList ScriptEditorWidget::completionWords() const
{
    return m_completionWords;
}

QStringList ScriptEditorWidget::lastMemberCompletionWordsForTesting() const
{
    return m_lastMemberCompletionWords;
}

void ScriptEditorWidget::setTextForTesting(const QString &text)
{
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor != nullptr) {
        m_editor->setText(text);
        const QStringList lines = text.split(QLatin1Char('\n'));
        const int line = qMax(0, lines.size() - 1);
        const int column = lines.isEmpty() ? 0 : lines.last().size();
        m_editor->setCursorPosition(line, column);
    }
#else
    Q_UNUSED(text)
#endif
}

void ScriptEditorWidget::triggerDotCompletionForTesting()
{
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor != nullptr) {
        insertDotAndShowMemberCompletion();
    }
#endif
}

void ScriptEditorWidget::typeMemberTextForTesting(const QString &text)
{
#if PYRAQT_HAS_QSCINTILLA
    for (const QChar ch : text) {
        insertTextAtCursor(QString(ch));
        showDotMemberCompletion();
    }
#else
    Q_UNUSED(text)
#endif
}

void ScriptEditorWidget::refreshMemberCompletionForTesting()
{
    showDotMemberCompletion();
}

void ScriptEditorWidget::newDocument()
{
#if PYRAQT_HAS_QSCINTILLA
    m_editor->setText(QString());
#endif
    m_currentFilePath.clear();
    setDocumentMode(DocumentMode::PlainText);
    emit filePathChanged(m_currentFilePath);
    setModified(false);
    emit cursorPositionChanged(currentLine(), currentColumn());
}

bool ScriptEditorWidget::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

#if PYRAQT_HAS_QSCINTILLA
    m_editor->setText(QString::fromUtf8(file.readAll()));
#endif
    m_currentFilePath = filePath;
    updateDocumentModeFromPath(filePath);
    emit filePathChanged(m_currentFilePath);
    setModified(false);
    emit cursorPositionChanged(currentLine(), currentColumn());
    return true;
}

bool ScriptEditorWidget::save()
{
    if (m_currentFilePath.isEmpty()) {
        return false;
    }
    return saveAs(m_currentFilePath);
}

bool ScriptEditorWidget::saveAs(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    file.write(currentText().toUtf8());
    m_currentFilePath = filePath;
    updateDocumentModeFromPath(filePath);
    emit filePathChanged(m_currentFilePath);
    setModified(false);
    emit cursorPositionChanged(currentLine(), currentColumn());
    return true;
}

void ScriptEditorWidget::setCurrentFilePath(const QString &filePath)
{
    if (m_currentFilePath == filePath) {
        return;
    }
    m_currentFilePath = filePath;
    updateDocumentModeFromPath(filePath);
    emit filePathChanged(m_currentFilePath);
}

void ScriptEditorWidget::setEditorMessage(const QString &message)
{
#if PYRAQT_HAS_QSCINTILLA
    Q_UNUSED(message)
#else
    if (m_placeholder != nullptr) {
        m_hasCustomPlaceholderMessage = true;
        m_placeholder->setText(message);
    }
#endif
}

void ScriptEditorWidget::retranslateUi()
{
#if PYRAQT_HAS_QSCINTILLA
#else
    if (m_placeholder != nullptr && !m_hasCustomPlaceholderMessage) {
        m_placeholder->setText(tr("Text editor unavailable on this build."));
    }
#endif
}

void ScriptEditorWidget::applyTheme(const QString &themeName)
{
    m_appliedTheme = themeName;

#if PYRAQT_HAS_QSCINTILLA
    const bool darkTheme = themeName == QStringLiteral("dark");
    const QColor background = darkTheme ? QColor(QStringLiteral("#0f1724")) : QColor(QStringLiteral("#ffffff"));
    const QColor foreground = darkTheme ? QColor(QStringLiteral("#e5edf7")) : QColor(QStringLiteral("#111827"));
    const QColor selection = darkTheme ? QColor(QStringLiteral("#3554b4")) : QColor(QStringLiteral("#c7d2fe"));
    const QColor caretLine = darkTheme ? QColor(QStringLiteral("#1b2436")) : QColor(QStringLiteral("#e8eef9"));
    const QColor marginBackground = darkTheme ? QColor(QStringLiteral("#161d2d")) : QColor(QStringLiteral("#f3f6fb"));
    const QColor marginForeground = darkTheme ? QColor(QStringLiteral("#93a4bd")) : QColor(QStringLiteral("#5f6f86"));
    const QColor braceBackground = darkTheme ? QColor(QStringLiteral("#223251")) : QColor(QStringLiteral("#dbe7ff"));
    const QColor braceForeground = darkTheme ? QColor(QStringLiteral("#f8fbff")) : QColor(QStringLiteral("#1f2937"));

    m_editor->setPaper(background);
    m_editor->setColor(foreground);
    m_editor->setSelectionBackgroundColor(selection);
    m_editor->setSelectionForegroundColor(foreground);
    m_editor->setCaretLineBackgroundColor(caretLine);
    m_editor->setCaretForegroundColor(foreground);
    m_editor->setMatchedBraceBackgroundColor(braceBackground);
    m_editor->setMatchedBraceForegroundColor(braceForeground);
    m_editor->setMarginsBackgroundColor(marginBackground);
    m_editor->setMarginsForegroundColor(marginForeground);
    m_editor->setFoldMarginColors(marginBackground, marginBackground);

    if (m_lexer != nullptr) {
        m_lexer->setDefaultPaper(background);
        m_lexer->setPaper(background);
        m_lexer->setColor(foreground);
        m_lexer->setDefaultColor(foreground);
    }
#else
    if (m_placeholder != nullptr) {
        QPalette palette = m_placeholder->palette();
        palette.setColor(QPalette::WindowText,
            themeName == QStringLiteral("dark") ? QColor(QStringLiteral("#e5edf7")) : QColor(QStringLiteral("#111827")));
        m_placeholder->setPalette(palette);
    }
#endif
}

void ScriptEditorWidget::setDocumentMode(const DocumentMode mode)
{
    m_documentMode = mode;
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor == nullptr) {
        return;
    }
    if (mode == DocumentMode::Python) {
        m_editor->setLexer(m_lexer);
        m_codeCompletionEnabled = m_apis != nullptr;
        m_dotCompletionEnabled = m_apis != nullptr;
    } else {
        if (m_editor->isListActive()) {
            m_editor->cancelList();
        }
        m_editor->setLexer(nullptr);
        m_codeCompletionEnabled = false;
        m_dotCompletionEnabled = false;
    }
#else
    Q_UNUSED(mode)
#endif
}

void ScriptEditorWidget::updateDocumentModeFromPath(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().trimmed().toLower();
    setDocumentMode(suffix == QStringLiteral("py") ? DocumentMode::Python : DocumentMode::PlainText);
}

void ScriptEditorWidget::configureCodeCompletion()
{
    pyraqt::core::PythonCompletionProvider provider;
    m_completionWords = provider.staticCompletions();
    m_editorMemberWords = provider.commonMemberNames();

#if PYRAQT_HAS_QSCINTILLA
    if (m_lexer == nullptr || m_editor == nullptr) {
        return;
    }
    m_apis = new QsciAPIs(m_lexer);
    for (const QString &word : m_completionWords) {
        m_apis->add(word);
    }
    m_apis->prepare();
    m_editor->setAutoCompletionSource(QsciScintilla::AcsAll);
    m_editor->setAutoCompletionThreshold(2);
    m_editor->setAutoCompletionCaseSensitivity(false);
    m_editor->setAutoCompletionReplaceWord(false);
    m_editor->setCallTipsStyle(QsciScintilla::CallTipsContext);
    m_codeCompletionEnabled = false;
    m_dotCompletionEnabled = false;
#else
    m_codeCompletionEnabled = false;
    m_dotCompletionEnabled = false;
#endif
}

QStringList ScriptEditorWidget::editorMemberCompletions() const
{
    QStringList words;
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor != nullptr && isPythonDocument()) {
        int line = 0;
        int column = 0;
        m_editor->getCursorPosition(&line, &column);
        const QString text = m_editor->text();
        const int cursor = m_editor->positionFromLineIndex(line, column);
        const QString prefix = core::PythonCompletionProvider::prefixBeforeCursor(text, cursor);
        const QString objectExpression = core::PythonCompletionProvider::objectExpressionForDottedPrefix(prefix);
        if (!objectExpression.isEmpty()) {
            if (m_runtimeManager != nullptr) {
                words = m_runtimeManager->completeMembers(objectExpression, contextCodeBeforeCursor(prefix.length()));
            }
            if (words.isEmpty()) {
                words = core::PythonCompletionProvider::directMembersForObject(m_completionWords, objectExpression);
            }
            const QString memberPrefix = core::PythonCompletionProvider::memberNamePrefixForDottedPrefix(prefix);
            if (!memberPrefix.isEmpty()) {
                QStringList filtered;
                for (const QString &word : words) {
                    if (word.startsWith(memberPrefix, Qt::CaseInsensitive)) {
                        filtered.push_back(word);
                    }
                }
                words = filtered;
            }
        }
    }
#endif
    if (words.isEmpty()) {
        words = m_editorMemberWords;
    }
    words.removeDuplicates();
    words.sort(Qt::CaseInsensitive);
    return words;
}

QString ScriptEditorWidget::contextCodeBeforeCursor(int prefixLength) const
{
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor != nullptr) {
        int line = 0;
        int column = 0;
        m_editor->getCursorPosition(&line, &column);
        const int cursor = m_editor->positionFromLineIndex(line, column);
        return m_editor->text().left(qMax(0, cursor - prefixLength));
    }
#endif
    return {};
}

void ScriptEditorWidget::showDotMemberCompletion()
{
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor == nullptr) {
        return;
    }
    const QStringList words = editorMemberCompletions();
    m_lastMemberCompletionWords = words;
    if (!words.isEmpty()) {
        m_editor->showUserList(1, words);
    }
#endif
}

void ScriptEditorWidget::insertDotAndShowMemberCompletion()
{
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor == nullptr) {
        return;
    }
    insertTextAtCursor(QStringLiteral("."));
    showDotMemberCompletion();
#endif
}

void ScriptEditorWidget::insertTextAtCursor(const QString &text)
{
#if PYRAQT_HAS_QSCINTILLA
    if (m_editor == nullptr || text.isEmpty()) {
        return;
    }
    int line = 0;
    int column = 0;
    m_editor->getCursorPosition(&line, &column);
    m_editor->insertAt(text, line, column);
    const QStringList lines = text.split(QLatin1Char('\n'));
    if (lines.size() == 1) {
        m_editor->setCursorPosition(line, column + text.size());
    } else {
        m_editor->setCursorPosition(line + lines.size() - 1, lines.last().size());
    }
#else
    Q_UNUSED(text)
#endif
}

void ScriptEditorWidget::setModified(bool modified)
{
    if (m_modified == modified) {
        return;
    }
    m_modified = modified;
    emit modificationChanged(modified);
}

bool ScriptEditorWidget::eventFilter(QObject *watched, QEvent *event)
{
#if PYRAQT_HAS_QSCINTILLA
    if (watched == m_editor && event->type() == QEvent::KeyPress) {
        if (!isPythonDocument()) {
            return QWidget::eventFilter(watched, event);
        }
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab && m_editor->isListActive()) {
            m_editor->SendScintilla(QsciScintillaBase::SCI_TAB);
            return true;
        }
        if (m_dotCompletionEnabled && keyEvent->key() == Qt::Key_Period && keyEvent->text() == QStringLiteral(".")) {
            if (m_editor->isListActive()) {
                m_editor->cancelList();
            }
            insertDotAndShowMemberCompletion();
            return true;
        }
        if (m_dotCompletionEnabled && !keyEvent->text().isEmpty()) {
            const QChar typed = keyEvent->text().at(0);
            if (typed.isLetterOrNumber() || typed == QLatin1Char('_')) {
                const QString before = core::PythonCompletionProvider::prefixBeforeCursor(
                    m_editor->text(),
                    m_editor->positionFromLineIndex(currentLine() - 1, currentColumn() - 1));
                if (before.contains(QLatin1Char('.'))) {
                    if (m_editor->isListActive()) {
                        m_editor->cancelList();
                    }
                    insertTextAtCursor(keyEvent->text());
                    showDotMemberCompletion();
                    return true;
                }
            }
        }
    }
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
#endif
    return QWidget::eventFilter(watched, event);
}

void ScriptEditorWidget::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

} // namespace pyraqt::ui
