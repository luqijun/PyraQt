#include "ui/editor/cad_document_widget.h"

#include "ui/editor/cad_summary_widget.h"
#include "ui/editor/occt_cad_view_widget.h"

#include <QStackedLayout>

namespace pyraqt::ui {

CadDocumentWidget::CadDocumentWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QStackedLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_viewWidget = new OcctCadViewWidget(this);
    m_errorSummaryWidget = new CadSummaryWidget(this);
    layout->addWidget(m_viewWidget);
    layout->addWidget(m_errorSummaryWidget);
    layout->setCurrentWidget(m_viewWidget);

    connect(m_viewWidget, &OcctCadViewWidget::displayModeChanged, this, &CadDocumentWidget::displayModeChanged);
    connect(m_viewWidget, &OcctCadViewWidget::selectionModeChanged, this, &CadDocumentWidget::selectionModeChanged);
    connect(m_viewWidget, &OcctCadViewWidget::selectionInfoChanged, this, &CadDocumentWidget::selectionInfoChanged);
    connect(m_viewWidget, &OcctCadViewWidget::hoverInfoChanged, this, &CadDocumentWidget::hoverInfoChanged);
    connect(m_viewWidget, &OcctCadViewWidget::statusMessageChanged, this, &CadDocumentWidget::statusMessageChanged);
}

void CadDocumentWidget::setDocument(const pyraqt::core::CadDocument &document)
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

void CadDocumentWidget::setDocumentFilePath(const QString &filePath)
{
    m_document.filePath = filePath;
    if (m_viewWidget != nullptr) {
        pyraqt::core::CadDocument updated = m_viewWidget->document();
        updated.filePath = filePath;
        m_viewWidget->setDocument(updated);
    }
}

void CadDocumentWidget::setDisplayMode(const pyraqt::core::CadDisplayMode mode)
{
    m_viewWidget->setDisplayMode(mode);
}

void CadDocumentWidget::setSelectionMode(const pyraqt::core::CadSelectionMode mode)
{
    m_viewWidget->setSelectionMode(mode);
}

void CadDocumentWidget::fitAll()
{
    m_viewWidget->fitAll();
}

void CadDocumentWidget::setStandardView(const QString &viewKey)
{
    m_viewWidget->setStandardView(viewKey);
}

void CadDocumentWidget::clearSelection()
{
    m_viewWidget->clearSelection();
}

pyraqt::core::CadDocument CadDocumentWidget::document() const
{
    return m_viewWidget->document().filePath.isEmpty() ? m_document : m_viewWidget->document();
}

pyraqt::core::CadSelectionInfo CadDocumentWidget::selectionInfo() const
{
    return m_viewWidget->selectionInfo();
}

OcctCadViewWidget *CadDocumentWidget::viewWidget() const
{
    return m_viewWidget;
}

} // namespace pyraqt::ui
