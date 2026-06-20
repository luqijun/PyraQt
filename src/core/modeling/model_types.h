#pragma once

#include <QString>
#include <QStringList>
#include <memory>

namespace pyraqt::core {

struct ModelOcctData;

enum class ModelFormat {
    Unknown,
    Step,
    Brep,
};

enum class ModelDisplayMode {
    Wireframe,
    Shaded,
    ShadedWithEdges,
};

enum class ModelSelectionMode {
    Shape,
    Face,
    Edge,
    Vertex,
};

enum class ModelSelectionKind {
    None,
    Model,
    Shape,
    Face,
    Edge,
    Vertex,
};

struct ModelImportSummary {
    QString filePath;
    ModelFormat format = ModelFormat::Unknown;
    bool isValid = false;
    int rootShapeCount = 0;
    int solidCount = 0;
    int shellCount = 0;
    int faceCount = 0;
    int edgeCount = 0;
    int vertexCount = 0;
    QString errorMessage;
};

struct ModelPropertyItem {
    QString label;
    QString value;
};

struct ModelMeasurementSummary {
    bool hasLength = false;
    double length = 0.0;
    bool hasArea = false;
    double area = 0.0;
    bool hasVolume = false;
    double volume = 0.0;
    QString centerOfMassText;
};

struct ModelDocument {
    QString filePath;
    ModelFormat format = ModelFormat::Unknown;
    bool isValid = false;
    ModelImportSummary summary;
    ModelDisplayMode displayMode = ModelDisplayMode::ShadedWithEdges;
    ModelSelectionMode selectionMode = ModelSelectionMode::Face;
    QString statusMessage;
    QString boundingBoxText;
    QStringList viewModes;
    ModelMeasurementSummary measurements;
    std::shared_ptr<ModelOcctData> occtData;
};

struct ModelSelectionInfo {
    ModelSelectionKind kind = ModelSelectionKind::None;
    QString label;
    QString boundingBoxText;
    QString measureText;
    ModelMeasurementSummary measurements;
    bool hasSelection = false;
};

} // namespace pyraqt::core
