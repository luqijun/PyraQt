#include "ui/common/python_completion_line_edit.h"

#include "core/scripting/python/python_completion_provider.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QStringListModel>
#include <utility>

namespace pyraqt::ui {

PythonCompletionLineEdit::PythonCompletionLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_completer(new QCompleter(this))
{
    m_completer->setWidget(this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    connect(m_completer, qOverload<const QString &>(&QCompleter::activated), this, &PythonCompletionLineEdit::insertCompletion);
}

void PythonCompletionLineEdit::setCompletionWords(const QStringList &words)
{
    m_completionWords = words;
    rebuildCompletionModel();
}

void PythonCompletionLineEdit::setMemberCompletionProvider(std::function<QStringList(const QString &)> provider)
{
    m_memberCompletionProvider = std::move(provider);
}

void PythonCompletionLineEdit::setCompletionThreshold(int threshold)
{
    m_completionThreshold = qMax(1, threshold);
}

bool PythonCompletionLineEdit::hasCompleter() const
{
    return m_completer != nullptr && m_completer->model() != nullptr;
}

QString PythonCompletionLineEdit::completionPrefixForTesting() const
{
    return m_completer == nullptr ? QString() : m_completer->completionPrefix();
}

QStringList PythonCompletionLineEdit::completionCandidatesForTesting() const
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

bool PythonCompletionLineEdit::applyCurrentCompletionForTesting()
{
    return applyCurrentCompletion();
}

bool PythonCompletionLineEdit::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if ((keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) && applyCurrentCompletion()) {
            event->accept();
            return true;
        }
    }
    return QLineEdit::event(event);
}

void PythonCompletionLineEdit::keyPressEvent(QKeyEvent *event)
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
        QLineEdit::keyPressEvent(event);
    }
    showCompletionIfNeeded(forceCompletion || dotCompletion);
}

void PythonCompletionLineEdit::focusInEvent(QFocusEvent *event)
{
    if (m_completer != nullptr) {
        m_completer->setWidget(this);
    }
    QLineEdit::focusInEvent(event);
}

bool PythonCompletionLineEdit::focusNextPrevChild(bool next)
{
    Q_UNUSED(next)
    if (applyCurrentCompletion()) {
        return true;
    }
    return false;
}

void PythonCompletionLineEdit::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != this) {
        return;
    }

    const QString prefix = core::PythonCompletionProvider::prefixBeforeCursor(text(), cursorPosition());
    const int start = cursorPosition() - prefix.length();
    setSelection(start, prefix.length());
    insert(completion);
}

bool PythonCompletionLineEdit::applyCurrentCompletion()
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

void PythonCompletionLineEdit::setExtraCompletionWords(const QStringList &words)
{
    m_extraCompletionWords = words;
    rebuildCompletionModel();
}

void PythonCompletionLineEdit::rebuildCompletionModel()
{
    QStringList words = m_completionWords;
    words.append(m_extraCompletionWords);
    words.removeDuplicates();
    words.sort(Qt::CaseInsensitive);
    m_completer->setModel(new QStringListModel(words, m_completer));
}

void PythonCompletionLineEdit::showCompletionIfNeeded(bool force)
{
    if (m_completer == nullptr || m_completer->model() == nullptr) {
        return;
    }

    const QString prefix = core::PythonCompletionProvider::prefixBeforeCursor(text(), cursorPosition());
    if (prefix.endsWith(QLatin1Char('.')) && m_memberCompletionProvider) {
        setExtraCompletionWords(m_memberCompletionProvider(prefix));
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
