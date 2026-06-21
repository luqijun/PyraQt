#include "ui/editor/editor_placeholder_widget.h"

#include <QEvent>
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

    m_titleLabel = new QLabel(title, panel);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setWordWrap(true);

    m_pathLabel = new QLabel(filePath, panel);
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QPalette pathPalette = m_pathLabel->palette();
    pathPalette.setColor(QPalette::WindowText, pathPalette.color(QPalette::WindowText).lighter(130));
    m_pathLabel->setPalette(pathPalette);

    m_messageLabel = new QLabel(message, panel);
    m_messageLabel->setWordWrap(true);

    panelLayout->addWidget(m_titleLabel);
    panelLayout->addWidget(m_pathLabel);
    panelLayout->addWidget(m_messageLabel);

    layout->addStretch();
    layout->addWidget(panel, 0, Qt::AlignCenter);
    layout->addStretch();
    refreshLabels();
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

void EditorPlaceholderWidget::setTitle(const QString &title)
{
    m_title = title;
    refreshLabels();
}

void EditorPlaceholderWidget::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
    refreshLabels();
}

void EditorPlaceholderWidget::setMessage(const QString &message)
{
    m_message = message;
    refreshLabels();
}

void EditorPlaceholderWidget::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        refreshLabels();
    }
    QWidget::changeEvent(event);
}

void EditorPlaceholderWidget::refreshLabels()
{
    if (m_titleLabel != nullptr) {
        m_titleLabel->setText(m_title);
    }
    if (m_pathLabel != nullptr) {
        m_pathLabel->setText(m_filePath);
    }
    if (m_messageLabel != nullptr) {
        m_messageLabel->setText(m_message);
    }
}

} // namespace pyraqt::ui
