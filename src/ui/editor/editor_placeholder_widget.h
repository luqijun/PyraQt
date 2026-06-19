#pragma once

#include <QWidget>

namespace pyraqt::ui {

class EditorPlaceholderWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EditorPlaceholderWidget(const QString &message, QWidget *parent = nullptr);
};

} // namespace pyraqt::ui
