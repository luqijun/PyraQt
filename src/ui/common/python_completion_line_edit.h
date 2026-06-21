#pragma once

#include <QLineEdit>
#include <functional>

class QCompleter;

namespace pyraqt::ui {

class PythonCompletionLineEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit PythonCompletionLineEdit(QWidget *parent = nullptr);

    void setCompletionWords(const QStringList &words);
    void setMemberCompletionProvider(std::function<QStringList(const QString &)> provider);
    void setCompletionThreshold(int threshold);
    [[nodiscard]] bool hasCompleter() const;
    [[nodiscard]] QString completionPrefixForTesting() const;
    [[nodiscard]] QStringList completionCandidatesForTesting() const;
    bool applyCurrentCompletionForTesting();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    bool focusNextPrevChild(bool next) override;

private slots:
    void insertCompletion(const QString &completion);

private:
    bool applyCurrentCompletion();
    void setExtraCompletionWords(const QStringList &words);
    void rebuildCompletionModel();
    void showCompletionIfNeeded(bool force);

    QCompleter *m_completer = nullptr;
    std::function<QStringList(const QString &)> m_memberCompletionProvider;
    QStringList m_completionWords;
    QStringList m_extraCompletionWords;
    int m_completionThreshold = 2;
};

} // namespace pyraqt::ui
