#include "core/modeling/model_property_service.h"

#include <QStringList>

namespace pyraqt::core {

QString ModelPropertyService::formatDisplayMode(const ModelDisplayMode mode)
{
    switch (mode) {
    case ModelDisplayMode::Wireframe:
        return QStringLiteral("Wireframe");
    case ModelDisplayMode::Shaded:
        return QStringLiteral("Shaded");
    case ModelDisplayMode::ShadedWithEdges:
        return QStringLiteral("Shaded With Edges");
    default:
        return QStringLiteral("Unknown");
    }
}

QString ModelPropertyService::formatSelectionMode(const ModelSelectionMode mode)
{
    switch (mode) {
    case ModelSelectionMode::Shape:
        return QStringLiteral("Shape");
    case ModelSelectionMode::Face:
        return QStringLiteral("Face");
    case ModelSelectionMode::Edge:
        return QStringLiteral("Edge");
    case ModelSelectionMode::Vertex:
        return QStringLiteral("Vertex");
    default:
        return QStringLiteral("Unknown");
    }
}

QString ModelPropertyService::formatSelectionType(const ModelSelectionKind kind)
{
    switch (kind) {
    case ModelSelectionKind::Model:
        return QStringLiteral("Model");
    case ModelSelectionKind::Shape:
        return QStringLiteral("Shape");
    case ModelSelectionKind::Face:
        return QStringLiteral("Face");
    case ModelSelectionKind::Edge:
        return QStringLiteral("Edge");
    case ModelSelectionKind::Vertex:
        return QStringLiteral("Vertex");
    default:
        return QStringLiteral("None");
    }
}

QString ModelPropertyService::formatModelFormat(const ModelFormat format)
{
    switch (format) {
    case ModelFormat::Step:
        return QStringLiteral("STEP");
    case ModelFormat::Brep:
        return QStringLiteral("BREP");
    default:
        return QStringLiteral("Unknown");
    }
}

QString ModelPropertyService::formatMeasureSummary(const ModelMeasurementSummary &measurements)
{
    QStringList parts;
    if (measurements.hasLength) {
        parts.push_back(QStringLiteral("L=%1").arg(measurements.length, 0, 'f', 3));
    }
    if (measurements.hasArea) {
        parts.push_back(QStringLiteral("A=%1").arg(measurements.area, 0, 'f', 3));
    }
    if (measurements.hasVolume) {
        parts.push_back(QStringLiteral("V=%1").arg(measurements.volume, 0, 'f', 3));
    }
    if (!measurements.centerOfMassText.isEmpty()) {
        parts.push_back(QStringLiteral("COM=%1").arg(measurements.centerOfMassText));
    }
    return parts.join(QStringLiteral(" | "));
}

} // namespace pyraqt::core
