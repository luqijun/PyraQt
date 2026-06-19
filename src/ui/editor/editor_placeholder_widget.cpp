#include "ui/editor/editor_placeholder_widget.h"

#include <QLabel>
#include <QVBoxLayout>

namespace pyraqt::ui {

EditorPlaceholderWidget::EditorPlaceholderWidget(const QString &message, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *label = new QLabel(message, this);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();
}

} // namespace pyraqt::ui
