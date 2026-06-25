#pragma once

#include <QString>
#include <QStringList>
#include <memory>

namespace pyraqt::core {

struct CadOcctData;

enum class CadFormat {
    Unknown,
    Step,
    Brep,
};

enum class CadDisplayMode {
    Wireframe,
    Shaded,
    ShadedWithEdges,
};

enum class CadSelectionMode {
    Shape,
    Face,
    Edge,
    Vertex,
};

enum class CadSelectionKind {
    None,
    Document,
    Shape,
    Face,
    Edge,
    Vertex,
};

struct CadImportSummary {
    QString filePath;
    CadFormat format = CadFormat::Unknown;
    bool isValid = false;
    int rootShapeCount = 0;
    int solidCount = 0;
    int shellCount = 0;
    int faceCount = 0;
    int edgeCount = 0;
    int vertexCount = 0;
    QString errorMessage;
};

struct CadPropertyItem {
    QString label;
    QString value;
};

struct CadMeasurementSummary {
    bool hasLength = false;
    double length = 0.0;
    bool hasArea = false;
    double area = 0.0;
    bool hasVolume = false;
    double volume = 0.0;
    QString centerOfMassText;
};

struct CadDocument {
    QString filePath;
    CadFormat format = CadFormat::Unknown;
    bool isValid = false;
    CadImportSummary summary;
    CadDisplayMode displayMode = CadDisplayMode::ShadedWithEdges;
    CadSelectionMode selectionMode = CadSelectionMode::Face;
    QString statusMessage;
    QString boundingBoxText;
    QStringList viewModes;
    CadMeasurementSummary measurements;
    std::shared_ptr<CadOcctData> occtData;
};

struct CadSelectionInfo {
    CadSelectionKind kind = CadSelectionKind::None;
    QString label;
    QString boundingBoxText;
    QString measureText;
    CadMeasurementSummary measurements;
    bool hasSelection = false;
};

} // namespace pyraqt::core
