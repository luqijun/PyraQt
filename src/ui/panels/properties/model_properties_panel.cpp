#include "ui/panels/properties/model_properties_panel.h"

#include "core/modeling/model_property_service.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace pyraqt::ui {

ModelPropertiesPanel::ModelPropertiesPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);

    auto *form = new QFormLayout();
    form->setContentsMargins(0, 0, 0, 0);

    m_stateLabel = new QLabel(this);
    m_fileLabel = new QLabel(this);
    m_formatLabel = new QLabel(this);
    m_boundsLabel = new QLabel(this);
    m_solidsLabel = new QLabel(this);
    m_facesLabel = new QLabel(this);
    m_edgesLabel = new QLabel(this);
    m_measureLabel = new QLabel(this);
    m_selectionTypeLabel = new QLabel(this);
    m_selectionLabel = new QLabel(this);
    m_selectionBoundsLabel = new QLabel(this);
    m_selectionMeasureLabel = new QLabel(this);

    form->addRow(tr("State"), m_stateLabel);
    form->addRow(tr("File"), m_fileLabel);
    form->addRow(tr("Format"), m_formatLabel);
    form->addRow(tr("Bounds"), m_boundsLabel);
    form->addRow(tr("Measures"), m_measureLabel);
    form->addRow(tr("Solids"), m_solidsLabel);
    form->addRow(tr("Faces"), m_facesLabel);
    form->addRow(tr("Edges"), m_edgesLabel);
    form->addRow(tr("Selection"), m_selectionTypeLabel);
    form->addRow(tr("Selection Label"), m_selectionLabel);
    form->addRow(tr("Selection Bounds"), m_selectionBoundsLabel);
    form->addRow(tr("Measure"), m_selectionMeasureLabel);

    layout->addLayout(form);
    showPlaceholder(tr("Select a model to inspect its properties."));
}

void ModelPropertiesPanel::setModelDocument(const pyraqt::core::ModelDocument &document)
{
    setValue(m_stateLabel, document.statusMessage);
    setValue(m_fileLabel, document.filePath);
    setValue(m_formatLabel, pyraqt::core::ModelPropertyService::formatModelFormat(document.summary.format));
    setValue(m_boundsLabel, document.boundingBoxText);
    setValue(m_measureLabel, pyraqt::core::ModelPropertyService::formatMeasureSummary(document.measurements));
    setValue(m_solidsLabel, QString::number(document.summary.solidCount));
    setValue(m_facesLabel, QString::number(document.summary.faceCount));
    setValue(m_edgesLabel, QString::number(document.summary.edgeCount));
}

void ModelPropertiesPanel::setSelectionInfo(const pyraqt::core::ModelSelectionInfo &selection)
{
    setValue(m_selectionTypeLabel, pyraqt::core::ModelPropertyService::formatSelectionType(selection.kind));
    setValue(m_selectionLabel, selection.label);
    setValue(m_selectionBoundsLabel, selection.boundingBoxText);
    const QString measureText = selection.measureText.isEmpty()
        ? pyraqt::core::ModelPropertyService::formatMeasureSummary(selection.measurements)
        : selection.measureText;
    setValue(m_selectionMeasureLabel, measureText);
}

void ModelPropertiesPanel::clearSelection()
{
    setValue(m_selectionTypeLabel, QStringLiteral("None"));
    setValue(m_selectionLabel, QString());
    setValue(m_selectionBoundsLabel, QString());
    setValue(m_selectionMeasureLabel, QString());
}

void ModelPropertiesPanel::showPlaceholder(const QString &message)
{
    setValue(m_stateLabel, message);
    setValue(m_fileLabel, QString());
    setValue(m_formatLabel, QString());
    setValue(m_boundsLabel, QString());
    setValue(m_measureLabel, QString());
    setValue(m_solidsLabel, QString());
    setValue(m_facesLabel, QString());
    setValue(m_edgesLabel, QString());
    clearSelection();
}

void ModelPropertiesPanel::setValue(QLabel *label, const QString &value)
{
    if (label != nullptr) {
        label->setText(value.isEmpty() ? tr("-") : value);
    }
}

} // namespace pyraqt::ui
