#include "core/cad/cad_import_manager.h"

#include "core/cad/cad_occt_data.h"

#include <QFileInfo>

#if PYRAQT_HAS_OCCT
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <STEPControl_Reader.hxx>
#include <AIS_Shape.hxx>
#include <TopAbs.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#endif

namespace pyraqt::core {

namespace {

QString normalizedSuffix(const QString &path)
{
    return QFileInfo(path).suffix().trimmed().toLower();
}

#if PYRAQT_HAS_OCCT
int countSubShapes(const TopoDS_Shape &shape, const TopAbs_ShapeEnum shapeType)
{
    int count = 0;
    for (TopExp_Explorer explorer(shape, shapeType); explorer.More(); explorer.Next()) {
        ++count;
    }
    return count;
}

QString boundingBoxText(const Bnd_Box &box)
{
    if (box.IsVoid()) {
        return QStringLiteral("Void");
    }

    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return QStringLiteral("[%1, %2, %3] - [%4, %5, %6]")
        .arg(xmin, 0, 'f', 3)
        .arg(ymin, 0, 'f', 3)
        .arg(zmin, 0, 'f', 3)
        .arg(xmax, 0, 'f', 3)
        .arg(ymax, 0, 'f', 3)
        .arg(zmax, 0, 'f', 3);
}

QString pointText(const gp_Pnt &point)
{
    return QStringLiteral("[%1, %2, %3]")
        .arg(point.X(), 0, 'f', 3)
        .arg(point.Y(), 0, 'f', 3)
        .arg(point.Z(), 0, 'f', 3);
}

CadMeasurementSummary measureShape(const TopoDS_Shape &shape)
{
    CadMeasurementSummary summary;

    if (shape.IsNull()) {
        return summary;
    }

    GProp_GProps linearProps;
    BRepGProp::LinearProperties(shape, linearProps);
    const Standard_Real length = linearProps.Mass();
    if (length > 0.0) {
        summary.hasLength = true;
        summary.length = length;
    }

    GProp_GProps surfaceProps;
    BRepGProp::SurfaceProperties(shape, surfaceProps);
    const Standard_Real area = surfaceProps.Mass();
    if (area > 0.0) {
        summary.hasArea = true;
        summary.area = area;
    }

    GProp_GProps volumeProps;
    BRepGProp::VolumeProperties(shape, volumeProps);
    const Standard_Real volume = volumeProps.Mass();
    if (volume > 0.0) {
        summary.hasVolume = true;
        summary.volume = volume;
        summary.centerOfMassText = pointText(volumeProps.CentreOfMass());
    } else if (summary.hasArea) {
        summary.centerOfMassText = pointText(surfaceProps.CentreOfMass());
    } else if (summary.hasLength) {
        summary.centerOfMassText = pointText(linearProps.CentreOfMass());
    }

    return summary;
}
#endif

} // namespace

CadImportManager::CadImportManager(QObject *parent)
    : QObject(parent)
{
}

bool CadImportManager::isOcctAvailable() const
{
#if PYRAQT_HAS_OCCT
    return true;
#else
    return false;
#endif
}

bool CadImportManager::isSupportedFile(const QString &path) const
{
    return detectFormat(path) != CadFormat::Unknown;
}

CadFormat CadImportManager::detectFormat(const QString &path) const
{
    const QString suffix = normalizedSuffix(path);
    if (suffix == QStringLiteral("stp") || suffix == QStringLiteral("step")) {
        return CadFormat::Step;
    }
    if (suffix == QStringLiteral("brep")) {
        return CadFormat::Brep;
    }
    return CadFormat::Unknown;
}

CadDocument CadImportManager::importFile(const QString &path) const
{
    CadDocument document;
    document.filePath = QFileInfo(path).absoluteFilePath();
    document.format = detectFormat(path);
    document.summary.filePath = document.filePath;
    document.summary.format = document.format;
    document.viewModes = QStringList{
        QStringLiteral("Wireframe"),
        QStringLiteral("Shaded"),
        QStringLiteral("Shaded With Edges"),
    };

    if (document.format == CadFormat::Unknown) {
        document.summary.errorMessage = tr("Unsupported model file format.");
        document.statusMessage = document.summary.errorMessage;
        return document;
    }

#if !PYRAQT_HAS_OCCT
    document.summary.errorMessage = tr("This build does not have OCCT model support enabled.");
    document.statusMessage = document.summary.errorMessage;
    return document;
#else
    TopoDS_Shape shape;

    if (document.format == CadFormat::Step) {
        STEPControl_Reader reader;
        const IFSelect_ReturnStatus status = reader.ReadFile(path.toUtf8().constData());
        if (status != IFSelect_RetDone) {
            document.summary.errorMessage = tr("Failed to read STEP file.");
            document.statusMessage = document.summary.errorMessage;
            return document;
        }

        const Standard_Integer transferCount = reader.TransferRoots();
        if (transferCount <= 0) {
            document.summary.errorMessage = tr("STEP file did not contain transferable roots.");
            document.statusMessage = document.summary.errorMessage;
            return document;
        }

        document.summary.rootShapeCount = reader.NbShapes();
        shape = reader.OneShape();
    } else {
        BRep_Builder builder;
        if (!BRepTools::Read(shape, path.toUtf8().constData(), builder)) {
            document.summary.errorMessage = tr("Failed to read BREP file.");
            document.statusMessage = document.summary.errorMessage;
            return document;
        }
        document.summary.rootShapeCount = shape.IsNull() ? 0 : 1;
    }

    if (shape.IsNull()) {
        document.summary.errorMessage = tr("Imported model shape is empty.");
        document.statusMessage = document.summary.errorMessage;
        return document;
    }

    document.occtData = std::make_shared<CadOcctData>();
    document.occtData->rootShape = shape;
    document.occtData->aisShape = new AIS_Shape(shape);
    document.summary.isValid = true;
    document.summary.solidCount = countSubShapes(shape, TopAbs_SOLID);
    document.summary.shellCount = countSubShapes(shape, TopAbs_SHELL);
    document.summary.faceCount = countSubShapes(shape, TopAbs_FACE);
    document.summary.edgeCount = countSubShapes(shape, TopAbs_EDGE);
    document.summary.vertexCount = countSubShapes(shape, TopAbs_VERTEX);

    BRepBndLib::Add(shape, document.occtData->boundingBox);
    document.boundingBoxText = boundingBoxText(document.occtData->boundingBox);
    document.measurements = measureShape(shape);
    document.isValid = true;
    document.statusMessage = tr("Model Loaded");
    return document;
#endif
}

} // namespace pyraqt::core
