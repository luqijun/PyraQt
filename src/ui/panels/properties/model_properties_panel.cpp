#include "ui/panels/properties/model_properties_panel.h"

#include "core/modeling/model_property_service.h"

#include <QEvent>
#include <QFormLayout>
#include <QFontMetrics>
#include <QLabel>
#include <QSizePolicy>
#include <QVariant>
#include <QVBoxLayout>

namespace pyraqt::ui {

namespace {

constexpr int kPropertiesMinimumWidth = 280;

}

ModelPropertiesPanel::ModelPropertiesPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("modelPropertiesPanel"));
    setMinimumWidth(kPropertiesMinimumWidth);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);

    m_form = new QFormLayout();
    m_form->setContentsMargins(0, 0, 0, 0);
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_stateLabel = createValueLabel(QStringLiteral("propertiesStateValue"));
    m_fileLabel = createValueLabel(QStringLiteral("propertiesFileValue"));
    m_formatLabel = createValueLabel(QStringLiteral("propertiesFormatValue"));
    m_boundsLabel = createValueLabel(QStringLiteral("propertiesBoundsValue"));
    m_solidsLabel = createValueLabel(QStringLiteral("propertiesSolidsValue"));
    m_facesLabel = createValueLabel(QStringLiteral("propertiesFacesValue"));
    m_edgesLabel = createValueLabel(QStringLiteral("propertiesEdgesValue"));
    m_measureLabel = createValueLabel(QStringLiteral("propertiesMeasureValue"));
    m_selectionTypeLabel = createValueLabel(QStringLiteral("propertiesSelectionTypeValue"));
    m_selectionLabel = createValueLabel(QStringLiteral("propertiesSelectionLabelValue"));
    m_selectionBoundsLabel = createValueLabel(QStringLiteral("propertiesSelectionBoundsValue"));
    m_selectionMeasureLabel = createValueLabel(QStringLiteral("propertiesSelectionMeasureValue"));

    m_form->addRow(QString(), m_stateLabel);
    m_form->addRow(QString(), m_fileLabel);
    m_form->addRow(QString(), m_formatLabel);
    m_form->addRow(QString(), m_boundsLabel);
    m_form->addRow(QString(), m_measureLabel);
    m_form->addRow(QString(), m_solidsLabel);
    m_form->addRow(QString(), m_facesLabel);
    m_form->addRow(QString(), m_edgesLabel);
    m_form->addRow(QString(), m_selectionTypeLabel);
    m_form->addRow(QString(), m_selectionLabel);
    m_form->addRow(QString(), m_selectionBoundsLabel);
    m_form->addRow(QString(), m_selectionMeasureLabel);

    layout->addLayout(m_form);
    retranslateUi();
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
        const QString displayValue = value.isEmpty() ? tr("-") : value;
        label->setProperty("fullText", QVariant(displayValue));
        label->setToolTip(displayValue);
        updateDisplayedValue(label);
    }
}

QLabel *ModelPropertiesPanel::createValueLabel(const QString &objectName)
{
    auto *label = new QLabel(this);
    label->setObjectName(objectName);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(false);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    label->setMinimumWidth(0);
    label->installEventFilter(this);
    return label;
}

bool ModelPropertiesPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (event != nullptr
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest)) {
        if (auto *label = qobject_cast<QLabel *>(watched)) {
            updateDisplayedValue(label);
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ModelPropertiesPanel::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void ModelPropertiesPanel::retranslateUi()
{
    if (m_form == nullptr) {
        return;
    }
    m_form->setWidget(0, QFormLayout::LabelRole, new QLabel(tr("State"), this));
    m_form->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("File"), this));
    m_form->setWidget(2, QFormLayout::LabelRole, new QLabel(tr("Format"), this));
    m_form->setWidget(3, QFormLayout::LabelRole, new QLabel(tr("Bounds"), this));
    m_form->setWidget(4, QFormLayout::LabelRole, new QLabel(tr("Measures"), this));
    m_form->setWidget(5, QFormLayout::LabelRole, new QLabel(tr("Solids"), this));
    m_form->setWidget(6, QFormLayout::LabelRole, new QLabel(tr("Faces"), this));
    m_form->setWidget(7, QFormLayout::LabelRole, new QLabel(tr("Edges"), this));
    m_form->setWidget(8, QFormLayout::LabelRole, new QLabel(tr("Selection"), this));
    m_form->setWidget(9, QFormLayout::LabelRole, new QLabel(tr("Selection Label"), this));
    m_form->setWidget(10, QFormLayout::LabelRole, new QLabel(tr("Selection Bounds"), this));
    m_form->setWidget(11, QFormLayout::LabelRole, new QLabel(tr("Measure"), this));
}

void ModelPropertiesPanel::updateDisplayedValue(QLabel *label)
{
    if (label == nullptr) {
        return;
    }

    const QString fullText = label->property("fullText").toString();
    const QString effectiveText = fullText.isEmpty() ? tr("-") : fullText;
    const int availableWidth = qMax(0, label->contentsRect().width());
    if (availableWidth <= 0) {
        label->setText(effectiveText);
        return;
    }

    const QFontMetrics metrics(label->font());
    label->setText(metrics.elidedText(effectiveText, Qt::ElideRight, availableWidth));
}

} // namespace pyraqt::ui
