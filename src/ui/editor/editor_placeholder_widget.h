#pragma once

#include <QWidget>

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
    void setFilePath(const QString &filePath);

private:
    QString m_title;
    QString m_filePath;
    QString m_message;
};

} // namespace pyraqt::ui
