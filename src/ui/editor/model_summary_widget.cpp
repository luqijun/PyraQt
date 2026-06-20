#include "ui/editor/model_summary_widget.h"

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

    auto *summaryLayout = new QFormLayout();
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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

    summaryLayout->addRow(tr("File"), m_fileValueLabel);
    summaryLayout->addRow(tr("Format"), m_formatValueLabel);
    summaryLayout->addRow(tr("Status"), m_statusValueLabel);
    summaryLayout->addRow(tr("Root Shapes"), m_rootShapesValueLabel);
    summaryLayout->addRow(tr("Solids"), m_solidsValueLabel);
    summaryLayout->addRow(tr("Shells"), m_shellsValueLabel);
    summaryLayout->addRow(tr("Faces"), m_facesValueLabel);
    summaryLayout->addRow(tr("Edges"), m_edgesValueLabel);
    summaryLayout->addRow(tr("Vertices"), m_verticesValueLabel);
    layout->addLayout(summaryLayout);

    m_errorViewer = new QPlainTextEdit(this);
    m_errorViewer->setReadOnly(true);
    m_errorViewer->setAccessibleName(tr("Model Import Details"));
    m_errorViewer->setAccessibleDescription(tr("Shows model import error details when loading fails."));
    layout->addWidget(m_errorViewer);

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

} // namespace pyraqt::ui
