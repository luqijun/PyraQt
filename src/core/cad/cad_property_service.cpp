#include "core/cad/cad_property_service.h"

#include <QStringList>

namespace pyraqt::core {

QString CadPropertyService::formatDisplayMode(const CadDisplayMode mode)
{
    switch (mode) {
    case CadDisplayMode::Wireframe:
        return QStringLiteral("Wireframe");
    case CadDisplayMode::Shaded:
        return QStringLiteral("Shaded");
    case CadDisplayMode::ShadedWithEdges:
        return QStringLiteral("Shaded With Edges");
    default:
        return QStringLiteral("Unknown");
    }
}

QString CadPropertyService::formatSelectionMode(const CadSelectionMode mode)
{
    switch (mode) {
    case CadSelectionMode::Shape:
        return QStringLiteral("Shape");
    case CadSelectionMode::Face:
        return QStringLiteral("Face");
    case CadSelectionMode::Edge:
        return QStringLiteral("Edge");
    case CadSelectionMode::Vertex:
        return QStringLiteral("Vertex");
    default:
        return QStringLiteral("Unknown");
    }
}

QString CadPropertyService::formatSelectionType(const CadSelectionKind kind)
{
    switch (kind) {
    case CadSelectionKind::Document:
        return QStringLiteral("Document");
    case CadSelectionKind::Shape:
        return QStringLiteral("Shape");
    case CadSelectionKind::Face:
        return QStringLiteral("Face");
    case CadSelectionKind::Edge:
        return QStringLiteral("Edge");
    case CadSelectionKind::Vertex:
        return QStringLiteral("Vertex");
    default:
        return QStringLiteral("None");
    }
}

QString CadPropertyService::formatFormat(const CadFormat format)
{
    switch (format) {
    case CadFormat::Step:
        return QStringLiteral("STEP");
    case CadFormat::Brep:
        return QStringLiteral("BREP");
    default:
        return QStringLiteral("Unknown");
    }
}

QString CadPropertyService::formatMeasureSummary(const CadMeasurementSummary &measurements)
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
