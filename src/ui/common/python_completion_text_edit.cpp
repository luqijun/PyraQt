#include "ui/common/python_completion_text_edit.h"

#include "core/scripting/python/python_completion_provider.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QKeyEvent>
#include <QModelIndex>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextCursor>
#include <utility>

namespace pyraqt::ui {

PythonCompletionTextEdit::PythonCompletionTextEdit(QWidget *parent)
    : QTextEdit(parent)
    , m_completer(new QCompleter(this))
{
    m_completer->setWidget(this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    connect(m_completer, qOverload<const QString &>(&QCompleter::activated), this, &PythonCompletionTextEdit::insertCompletion);
}

void PythonCompletionTextEdit::setCompletionWords(const QStringList &words)
{
    m_completionWords = words;
    rebuildCompletionModel();
}

void PythonCompletionTextEdit::setMemberCompletionProvider(std::function<QStringList(const QString &, const QString &)> provider)
{
    m_memberCompletionProvider = std::move(provider);
}

void PythonCompletionTextEdit::setExtraCompletionWords(const QStringList &words)
{
    m_extraCompletionWords = words;
    rebuildCompletionModel();
}

void PythonCompletionTextEdit::setCompletionThreshold(int threshold)
{
    m_completionThreshold = qMax(1, threshold);
}

bool PythonCompletionTextEdit::hasCompleter() const
{
    return m_completer != nullptr && m_completer->model() != nullptr;
}

QStringList PythonCompletionTextEdit::completionWords() const
{
    return m_completionWords;
}

QStringList PythonCompletionTextEdit::completionCandidatesForTesting() const
{
    QStringList candidates;
    if (m_completer == nullptr || m_completer->model() == nullptr) {
        return candidates;
    }
    QAbstractItemModel *model = m_completer->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        candidates.push_back(model->index(row, 0).data().toString());
    }
    return candidates;
}

QString PythonCompletionTextEdit::completionPrefixForTesting() const
{
    return m_completer == nullptr ? QString() : m_completer->completionPrefix();
}

bool PythonCompletionTextEdit::applyCurrentCompletionForTesting()
{
    return applyCurrentCompletion();
}

void PythonCompletionTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (m_completer != nullptr && m_completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            if (applyCurrentCompletion()) {
                event->accept();
                return;
            }
            event->ignore();
            return;
        case Qt::Key_Escape:
            event->ignore();
            return;
        default:
            break;
        }
    }

    const bool forceCompletion = event->modifiers().testFlag(Qt::ControlModifier) && event->key() == Qt::Key_Space;
    const bool dotCompletion = event->key() == Qt::Key_Period && event->text() == QStringLiteral(".");
    if (!forceCompletion) {
        QTextEdit::keyPressEvent(event);
    }
    showCompletionIfNeeded(forceCompletion || dotCompletion);
}

void PythonCompletionTextEdit::focusInEvent(QFocusEvent *event)
{
    if (m_completer != nullptr) {
        m_completer->setWidget(this);
    }
    QTextEdit::focusInEvent(event);
}

void PythonCompletionTextEdit::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != this) {
        return;
    }

    const QString prefix = completionPrefixUnderCursor();
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefix.length());
    cursor.insertText(completion);
    setTextCursor(cursor);
}

bool PythonCompletionTextEdit::applyCurrentCompletion()
{
    if (m_completer == nullptr || !m_completer->popup()->isVisible()) {
        return false;
    }

    const QModelIndex index = m_completer->popup()->currentIndex();
    if (!index.isValid()) {
        return false;
    }
    insertCompletion(index.data().toString());
    m_completer->popup()->hide();
    return true;
}

QString PythonCompletionTextEdit::completionPrefixUnderCursor() const
{
    const QTextCursor cursor = textCursor();
    return pyraqt::core::PythonCompletionProvider::prefixBeforeCursor(toPlainText(), cursor.position());
}

QString PythonCompletionTextEdit::contextBeforeCompletionPrefix(const QString &prefix) const
{
    const QTextCursor cursor = textCursor();
    return toPlainText().left(qMax(0, cursor.position() - prefix.length()));
}

void PythonCompletionTextEdit::rebuildCompletionModel()
{
    QStringList words = m_completionWords;
    words.append(m_extraCompletionWords);
    words.removeDuplicates();
    words.sort(Qt::CaseInsensitive);
    auto *model = new QStringListModel(words, m_completer);
    m_completer->setModel(model);
}

void PythonCompletionTextEdit::showCompletionIfNeeded(bool force)
{
    if (m_completer == nullptr || m_completer->model() == nullptr) {
        return;
    }

    const QString prefix = completionPrefixUnderCursor();
    if (prefix.contains(QLatin1Char('.')) && m_memberCompletionProvider) {
        setExtraCompletionWords(m_memberCompletionProvider(prefix, contextBeforeCompletionPrefix(prefix)));
    }

    if (!force && prefix.length() < m_completionThreshold && !prefix.endsWith(QLatin1Char('.'))) {
        m_completer->popup()->hide();
        return;
    }

    if (prefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(prefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    QRect rect = cursorRect();
    rect.setWidth(m_completer->popup()->sizeHintForColumn(0) + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(rect);
}

} // namespace pyraqt::ui
