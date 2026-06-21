#include "ui/editor/editor_placeholder_widget.h"

#include <QFrame>
#include <QLabel>
#include <QPalette>
#include <QVBoxLayout>

namespace pyraqt::ui {

EditorPlaceholderWidget::EditorPlaceholderWidget(
    const QString &title,
    const QString &filePath,
    const QString &message,
    QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_filePath(filePath)
    , m_message(message)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("previewUnavailablePanel"));
    panel->setFrameShape(QFrame::StyledPanel);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(24, 24, 24, 24);
    panelLayout->setSpacing(12);

    auto *titleLabel = new QLabel(title, panel);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleLabel->setFont(titleFont);
    titleLabel->setWordWrap(true);

    auto *pathLabel = new QLabel(filePath, panel);
    pathLabel->setWordWrap(true);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QPalette pathPalette = pathLabel->palette();
    pathPalette.setColor(QPalette::WindowText, pathPalette.color(QPalette::WindowText).lighter(130));
    pathLabel->setPalette(pathPalette);

    auto *messageLabel = new QLabel(message, panel);
    messageLabel->setWordWrap(true);

    panelLayout->addWidget(titleLabel);
    panelLayout->addWidget(pathLabel);
    panelLayout->addWidget(messageLabel);

    layout->addStretch();
    layout->addWidget(panel, 0, Qt::AlignCenter);
    layout->addStretch();
}

QString EditorPlaceholderWidget::title() const
{
    return m_title;
}

QString EditorPlaceholderWidget::filePath() const
{
    return m_filePath;
}

QString EditorPlaceholderWidget::message() const
{
    return m_message;
}

void EditorPlaceholderWidget::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}

} // namespace pyraqt::ui
