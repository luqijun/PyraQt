#include "ui/editor/script_editor_widget.h"

#include <QColor>
#include <QFile>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

#if PYRAQT_HAS_QSCINTILLA
#include <Qsci/qscilexerpython.h>
#include <Qsci/qsciscintilla.h>
#endif

namespace pyraqt::ui {

ScriptEditorWidget::ScriptEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

#if PYRAQT_HAS_QSCINTILLA
    m_editor = new QsciScintilla(this);
    m_lexer = new QsciLexerPython(this);
    m_editor->setLexer(m_lexer);
    m_editor->setUtf8(true);
    m_editor->setIndentationWidth(4);
    m_editor->setAutoIndent(true);
    m_editor->setMarginLineNumbers(1, true);
    m_editor->setMarginWidth(1, QStringLiteral("00000"));
    m_editor->setBraceMatching(QsciScintilla::StrictBraceMatch);
    m_editor->setCaretLineVisible(true);
    m_editor->setCaretLineBackgroundColor(QColor(QStringLiteral("#1d2433")));
    m_editor->setText(QStringLiteral("# PyraQt script\nimport pyra\n\npyra.log.info('Ready to script')\n"));
    connect(m_editor, &QsciScintilla::textChanged, this, [this] {
        setModified(true);
    });
    connect(m_editor, &QsciScintilla::cursorPositionChanged, this, [this](int line, int column) {
        emit cursorPositionChanged(line + 1, column + 1);
    });
    layout->addWidget(m_editor);
#else
    m_placeholder = new QLabel(tr("Phase 2 editor unavailable on this build."), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);
    layout->addWidget(m_placeholder);
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

void ScriptEditorWidget::newDocument()
{
#if PYRAQT_HAS_QSCINTILLA
    m_editor->setText(QStringLiteral("# New PyraQt script\nimport pyra\n"));
#endif
    m_currentFilePath.clear();
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
    emit filePathChanged(m_currentFilePath);
    setModified(false);
    emit cursorPositionChanged(currentLine(), currentColumn());
    return true;
}

void ScriptEditorWidget::setEditorMessage(const QString &message)
{
#if PYRAQT_HAS_QSCINTILLA
    Q_UNUSED(message)
#else
    if (m_placeholder != nullptr) {
        m_placeholder->setText(message);
    }
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

} // namespace pyraqt::ui
