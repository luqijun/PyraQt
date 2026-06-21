#pragma once

#include <QTextEdit>
#include <functional>

class QCompleter;

namespace pyraqt::ui {

class PythonCompletionTextEdit final : public QTextEdit {
    Q_OBJECT

public:
    explicit PythonCompletionTextEdit(QWidget *parent = nullptr);

    void setCompletionWords(const QStringList &words);
    void setMemberCompletionProvider(std::function<QStringList(const QString &, const QString &)> provider);
    void setExtraCompletionWords(const QStringList &words);
    void setCompletionThreshold(int threshold);
    [[nodiscard]] bool hasCompleter() const;
    [[nodiscard]] QStringList completionWords() const;
    [[nodiscard]] QStringList completionCandidatesForTesting() const;
    [[nodiscard]] QString completionPrefixForTesting() const;
    bool applyCurrentCompletionForTesting();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void insertCompletion(const QString &completion);

private:
    bool applyCurrentCompletion();
    [[nodiscard]] QString completionPrefixUnderCursor() const;
    [[nodiscard]] QString contextBeforeCompletionPrefix(const QString &prefix) const;
    void rebuildCompletionModel();
    void showCompletionIfNeeded(bool force);

    QCompleter *m_completer = nullptr;
    std::function<QStringList(const QString &, const QString &)> m_memberCompletionProvider;
    QStringList m_completionWords;
    QStringList m_extraCompletionWords;
    int m_completionThreshold = 2;
};

} // namespace pyraqt::ui
