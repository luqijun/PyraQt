#include "ui/editor/model_summary_widget.h"

#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace pyraqt::ui {
namespace {

QString formatText(const pyraqt::core::ModelFormat format)
{
    switch (format) {
    case pyraqt::core::ModelFormat::Step:
        return QStringLiteral("STEP");
    case pyraqt::core::ModelFormat::Brep:
        return QStringLiteral("BREP");
    case pyraqt::core::ModelFormat::Unknown:
    default:
        return QStringLiteral("Unknown");
    }
}

QString countText(const int value)
{
    return QString::number(value);
}

} // namespace

ModelSummaryWidget::ModelSummaryWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);

    m_summaryLayout = new QFormLayout();
    m_summaryLayout->setContentsMargins(0, 0, 0, 0);
    m_summaryLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_fileValueLabel = new QLabel(this);
    m_fileValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_formatValueLabel = new QLabel(this);
    m_statusValueLabel = new QLabel(this);
    m_rootShapesValueLabel = new QLabel(this);
    m_solidsValueLabel = new QLabel(this);
    m_shellsValueLabel = new QLabel(this);
    m_facesValueLabel = new QLabel(this);
    m_edgesValueLabel = new QLabel(this);
    m_verticesValueLabel = new QLabel(this);

    m_summaryLayout->addRow(QString(), m_fileValueLabel);
    m_summaryLayout->addRow(QString(), m_formatValueLabel);
    m_summaryLayout->addRow(QString(), m_statusValueLabel);
    m_summaryLayout->addRow(QString(), m_rootShapesValueLabel);
    m_summaryLayout->addRow(QString(), m_solidsValueLabel);
    m_summaryLayout->addRow(QString(), m_shellsValueLabel);
    m_summaryLayout->addRow(QString(), m_facesValueLabel);
    m_summaryLayout->addRow(QString(), m_edgesValueLabel);
    m_summaryLayout->addRow(QString(), m_verticesValueLabel);
    layout->addLayout(m_summaryLayout);

    m_errorViewer = new QPlainTextEdit(this);
    m_errorViewer->setReadOnly(true);
    layout->addWidget(m_errorViewer);

    retranslateUi();
    setSummary({});
}

void ModelSummaryWidget::setSummary(const pyraqt::core::ModelImportSummary &summary)
{
    m_summary = summary;
    m_fileValueLabel->setText(summary.filePath.isEmpty() ? tr("(unspecified)") : summary.filePath);
    m_formatValueLabel->setText(formatText(summary.format));
    m_statusValueLabel->setText(summary.isValid ? tr("Model Loaded") : tr("Load Failed"));
    m_rootShapesValueLabel->setText(countText(summary.rootShapeCount));
    m_solidsValueLabel->setText(countText(summary.solidCount));
    m_shellsValueLabel->setText(countText(summary.shellCount));
    m_facesValueLabel->setText(countText(summary.faceCount));
    m_edgesValueLabel->setText(countText(summary.edgeCount));
    m_verticesValueLabel->setText(countText(summary.vertexCount));
    m_errorViewer->setPlainText(summary.errorMessage.isEmpty() ? tr("No import errors.") : summary.errorMessage);
}

pyraqt::core::ModelImportSummary ModelSummaryWidget::summary() const
{
    return m_summary;
}

void ModelSummaryWidget::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
        setSummary(m_summary);
    }
    QWidget::changeEvent(event);
}

void ModelSummaryWidget::retranslateUi()
{
    if (m_summaryLayout != nullptr) {
        m_summaryLayout->setWidget(0, QFormLayout::LabelRole, new QLabel(tr("File"), this));
        m_summaryLayout->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("Format"), this));
        m_summaryLayout->setWidget(2, QFormLayout::LabelRole, new QLabel(tr("Status"), this));
        m_summaryLayout->setWidget(3, QFormLayout::LabelRole, new QLabel(tr("Root Shapes"), this));
        m_summaryLayout->setWidget(4, QFormLayout::LabelRole, new QLabel(tr("Solids"), this));
        m_summaryLayout->setWidget(5, QFormLayout::LabelRole, new QLabel(tr("Shells"), this));
        m_summaryLayout->setWidget(6, QFormLayout::LabelRole, new QLabel(tr("Faces"), this));
        m_summaryLayout->setWidget(7, QFormLayout::LabelRole, new QLabel(tr("Edges"), this));
        m_summaryLayout->setWidget(8, QFormLayout::LabelRole, new QLabel(tr("Vertices"), this));
    }
    if (m_errorViewer != nullptr) {
        m_errorViewer->setAccessibleName(tr("Model Import Details"));
        m_errorViewer->setAccessibleDescription(tr("Shows model import error details when loading fails."));
    }
}

} // namespace pyraqt::ui
