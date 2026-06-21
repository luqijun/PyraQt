#include "ui/editor/model_document_widget.h"

#include "ui/editor/model_summary_widget.h"
#include "ui/editor/occt_model_view_widget.h"

#include <QStackedLayout>

namespace pyraqt::ui {

ModelDocumentWidget::ModelDocumentWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QStackedLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_viewWidget = new OcctModelViewWidget(this);
    m_errorSummaryWidget = new ModelSummaryWidget(this);
    layout->addWidget(m_viewWidget);
    layout->addWidget(m_errorSummaryWidget);
    layout->setCurrentWidget(m_viewWidget);

    connect(m_viewWidget, &OcctModelViewWidget::displayModeChanged, this, &ModelDocumentWidget::displayModeChanged);
    connect(m_viewWidget, &OcctModelViewWidget::selectionModeChanged, this, &ModelDocumentWidget::selectionModeChanged);
    connect(m_viewWidget, &OcctModelViewWidget::selectionInfoChanged, this, &ModelDocumentWidget::selectionInfoChanged);
    connect(m_viewWidget, &OcctModelViewWidget::hoverInfoChanged, this, &ModelDocumentWidget::hoverInfoChanged);
    connect(m_viewWidget, &OcctModelViewWidget::statusMessageChanged, this, &ModelDocumentWidget::statusMessageChanged);
}

void ModelDocumentWidget::setDocument(const pyraqt::core::ModelDocument &document)
{
    m_document = document;
    auto *layout = qobject_cast<QStackedLayout *>(this->layout());
    if (document.isValid) {
        m_viewWidget->setDocument(document);
        if (layout != nullptr) {
            layout->setCurrentWidget(m_viewWidget);
        }
    } else {
        m_errorSummaryWidget->setSummary(document.summary);
        if (layout != nullptr) {
            layout->setCurrentWidget(m_errorSummaryWidget);
        }
    }
}

void ModelDocumentWidget::setDocumentFilePath(const QString &filePath)
{
    m_document.filePath = filePath;
    if (m_viewWidget != nullptr) {
        pyraqt::core::ModelDocument updated = m_viewWidget->document();
        updated.filePath = filePath;
        m_viewWidget->setDocument(updated);
    }
}

void ModelDocumentWidget::setDisplayMode(const pyraqt::core::ModelDisplayMode mode)
{
    m_viewWidget->setDisplayMode(mode);
}

void ModelDocumentWidget::setSelectionMode(const pyraqt::core::ModelSelectionMode mode)
{
    m_viewWidget->setSelectionMode(mode);
}

void ModelDocumentWidget::fitAll()
{
    m_viewWidget->fitAll();
}

void ModelDocumentWidget::setStandardView(const QString &viewName)
{
    m_viewWidget->setStandardView(viewName);
}

void ModelDocumentWidget::clearSelection()
{
    m_viewWidget->clearSelection();
}

pyraqt::core::ModelDocument ModelDocumentWidget::document() const
{
    return m_viewWidget->document().filePath.isEmpty() ? m_document : m_viewWidget->document();
}

pyraqt::core::ModelSelectionInfo ModelDocumentWidget::selectionInfo() const
{
    return m_viewWidget->selectionInfo();
}

OcctModelViewWidget *ModelDocumentWidget::viewWidget() const
{
    return m_viewWidget;
}

} // namespace pyraqt::ui
