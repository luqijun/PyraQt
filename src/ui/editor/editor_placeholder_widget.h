#pragma once

#include <QWidget>

class QEvent;
class QLabel;

namespace pyraqt::ui {

class EditorPlaceholderWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EditorPlaceholderWidget(
        const QString &title,
        const QString &filePath,
        const QString &message,
        QWidget *parent = nullptr);

    [[nodiscard]] QString title() const;
    [[nodiscard]] QString filePath() const;
    [[nodiscard]] QString message() const;
    void setTitle(const QString &title);
    void setFilePath(const QString &filePath);
    void setMessage(const QString &message);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshLabels();

    QString m_title;
    QString m_filePath;
    QString m_message;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_pathLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
};

} // namespace pyraqt::ui
