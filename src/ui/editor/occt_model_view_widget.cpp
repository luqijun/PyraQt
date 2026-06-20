#include "ui/editor/occt_model_view_widget.h"

#include "core/modeling/model_occt_data.h"
#include "core/modeling/model_property_service.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEngine>
#include <QResizeEvent>
#include <QWheelEvent>

#if PYRAQT_HAS_OCCT
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_Drawer.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_TypeOfOrientation.hxx>
#endif

namespace pyraqt::ui {

namespace {

#if PYRAQT_HAS_OCCT
QString pointText(const double x, const double y, const double z)
{
    return QStringLiteral("[%1, %2, %3]")
        .arg(x, 0, 'f', 3)
        .arg(y, 0, 'f', 3)
        .arg(z, 0, 'f', 3);
}

QString boundsTextFromMinMax(
    const double xmin,
    const double ymin,
    const double zmin,
    const double xmax,
    const double ymax,
    const double zmax)
{
    return QStringLiteral("[%1, %2, %3] - [%4, %5, %6]")
        .arg(xmin, 0, 'f', 3)
        .arg(ymin, 0, 'f', 3)
        .arg(zmin, 0, 'f', 3)
        .arg(xmax, 0, 'f', 3)
        .arg(ymax, 0, 'f', 3)
        .arg(zmax, 0, 'f', 3);
}

pyraqt::core::ModelMeasurementSummary measureShape(const TopoDS_Shape &shape)
{
    pyraqt::core::ModelMeasurementSummary summary;
    if (shape.IsNull()) {
        return summary;
    }

    GProp_GProps linearProps;
    BRepGProp::LinearProperties(shape, linearProps);
    if (linearProps.Mass() > 0.0) {
        summary.hasLength = true;
        summary.length = linearProps.Mass();
    }

    GProp_GProps surfaceProps;
    BRepGProp::SurfaceProperties(shape, surfaceProps);
    if (surfaceProps.Mass() > 0.0) {
        summary.hasArea = true;
        summary.area = surfaceProps.Mass();
    }

    GProp_GProps volumeProps;
    BRepGProp::VolumeProperties(shape, volumeProps);
    if (volumeProps.Mass() > 0.0) {
        summary.hasVolume = true;
        summary.volume = volumeProps.Mass();
        const gp_Pnt center = volumeProps.CentreOfMass();
        summary.centerOfMassText = pointText(center.X(), center.Y(), center.Z());
    } else if (summary.hasArea) {
        const gp_Pnt center = surfaceProps.CentreOfMass();
        summary.centerOfMassText = pointText(center.X(), center.Y(), center.Z());
    } else if (summary.hasLength) {
        const gp_Pnt center = linearProps.CentreOfMass();
        summary.centerOfMassText = pointText(center.X(), center.Y(), center.Z());
    }

    return summary;
}

QString shapeLabel(const TopoDS_Shape &shape)
{
    switch (shape.ShapeType()) {
    case TopAbs_FACE:
        return QStringLiteral("Face");
    case TopAbs_EDGE:
        return QStringLiteral("Edge");
    case TopAbs_VERTEX:
        return QStringLiteral("Vertex");
    case TopAbs_SOLID:
        return QStringLiteral("Solid");
    case TopAbs_SHELL:
        return QStringLiteral("Shell");
    case TopAbs_WIRE:
        return QStringLiteral("Wire");
    case TopAbs_COMPOUND:
        return QStringLiteral("Compound");
    case TopAbs_COMPSOLID:
        return QStringLiteral("CompSolid");
    default:
        return QStringLiteral("Shape");
    }
}

pyraqt::core::ModelSelectionKind selectionKindFromShape(const TopoDS_Shape &shape)
{
    switch (shape.ShapeType()) {
    case TopAbs_FACE:
        return pyraqt::core::ModelSelectionKind::Face;
    case TopAbs_EDGE:
        return pyraqt::core::ModelSelectionKind::Edge;
    case TopAbs_VERTEX:
        return pyraqt::core::ModelSelectionKind::Vertex;
    default:
        return pyraqt::core::ModelSelectionKind::Shape;
    }
}

int occtSelectionMode(const pyraqt::core::ModelSelectionMode mode)
{
    switch (mode) {
    case pyraqt::core::ModelSelectionMode::Vertex:
        return AIS_Shape::SelectionMode(TopAbs_VERTEX);
    case pyraqt::core::ModelSelectionMode::Edge:
        return AIS_Shape::SelectionMode(TopAbs_EDGE);
    case pyraqt::core::ModelSelectionMode::Face:
        return AIS_Shape::SelectionMode(TopAbs_FACE);
    case pyraqt::core::ModelSelectionMode::Shape:
    default:
        return AIS_Shape::SelectionMode(TopAbs_SHAPE);
    }
}

V3d_TypeOfOrientation orientationForName(const QString &viewName)
{
    const QString normalized = viewName.trimmed().toLower();
    if (normalized.contains(QStringLiteral("front"))) {
        return V3d_TypeOfOrientation_Zup_Front;
    }
    if (normalized.contains(QStringLiteral("back"))) {
        return V3d_TypeOfOrientation_Zup_Back;
    }
    if (normalized.contains(QStringLiteral("left"))) {
        return V3d_TypeOfOrientation_Zup_Left;
    }
    if (normalized.contains(QStringLiteral("right"))) {
        return V3d_TypeOfOrientation_Zup_Right;
    }
    if (normalized.contains(QStringLiteral("top"))) {
        return V3d_TypeOfOrientation_Zup_Top;
    }
    if (normalized.contains(QStringLiteral("bottom"))) {
        return V3d_TypeOfOrientation_Zup_Bottom;
    }
    return V3d_TypeOfOrientation_Zup_AxoRight;
}
#endif

} // namespace

OcctModelViewWidget::OcctModelViewWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(240, 180);
}

OcctModelViewWidget::~OcctModelViewWidget() = default;

void OcctModelViewWidget::setDocument(const pyraqt::core::ModelDocument &document)
{
    m_document = document;
    m_selectionInfo = {};

    if (isViewerReady()) {
        displayDocument();
    }

    emit displayModeChanged(m_document.displayMode);
    emit selectionModeChanged(m_document.selectionMode);
    emit statusMessageChanged(document.statusMessage);
    emit selectionInfoChanged(m_selectionInfo);
}

void OcctModelViewWidget::setDisplayMode(const pyraqt::core::ModelDisplayMode mode)
{
    m_document.displayMode = mode;
    if (isViewerReady()) {
        applyDisplayMode();
    }
    emit displayModeChanged(mode);
    emit statusMessageChanged(pyraqt::core::ModelPropertyService::formatDisplayMode(mode));
}

void OcctModelViewWidget::setSelectionMode(const pyraqt::core::ModelSelectionMode mode)
{
    m_document.selectionMode = mode;
    if (isViewerReady()) {
        applySelectionMode();
    }
    emit selectionModeChanged(mode);
    refreshSelectionInfo();
    emit statusMessageChanged(pyraqt::core::ModelPropertyService::formatSelectionMode(mode));
}

void OcctModelViewWidget::fitAll()
{
#if PYRAQT_HAS_OCCT
    if (m_view.IsNull()) {
        return;
    }

    m_view->FitAll(0.01, Standard_True);
    m_view->ZFitAll();
    m_view->Redraw();
#endif
    emit statusMessageChanged(tr("Fit All"));
}

void OcctModelViewWidget::setStandardView(const QString &viewName)
{
#if PYRAQT_HAS_OCCT
    if (!m_view.IsNull()) {
        m_view->SetProj(orientationForName(viewName));
        m_view->FitAll(0.01, Standard_True);
        m_view->ZFitAll();
        m_view->Redraw();
    }
#endif
    emit statusMessageChanged(tr("View: %1").arg(viewName));
}

void OcctModelViewWidget::clearSelection()
{
#if PYRAQT_HAS_OCCT
    if (!m_context.IsNull()) {
        m_context->ClearSelected(Standard_True);
        if (!m_view.IsNull()) {
            m_view->Redraw();
        }
    }
#endif
    m_selectionInfo = {};
    emit selectionInfoChanged(m_selectionInfo);
}

pyraqt::core::ModelDocument OcctModelViewWidget::document() const
{
    return m_document;
}

pyraqt::core::ModelSelectionInfo OcctModelViewWidget::selectionInfo() const
{
    return m_selectionInfo;
}

void OcctModelViewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    initializeViewer();
    if (isViewerReady()) {
        displayDocument();
    }
}

void OcctModelViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
#if PYRAQT_HAS_OCCT
    if (!m_window.IsNull()) {
        Handle(Aspect_NeutralWindow) neutralWindow = Handle(Aspect_NeutralWindow)::DownCast(m_window);
        if (!neutralWindow.IsNull()) {
            neutralWindow->SetSize(width(), height());
        }
    }
    if (!m_view.IsNull()) {
        m_view->MustBeResized();
        m_view->Invalidate();
        m_view->Redraw();
    }
#endif
}

void OcctModelViewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

#if PYRAQT_HAS_OCCT
    if (isViewerReady()) {
        m_view->Redraw();
        return;
    }
#endif

    QPainter painter(this);
    painter.fillRect(rect(), palette().window());
    painter.setPen(palette().text().color());
    painter.drawText(rect(), Qt::AlignCenter, tr("OCCT viewer unavailable"));
}

void OcctModelViewWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

#if PYRAQT_HAS_OCCT
    if (!isViewerReady()) {
        return;
    }

    m_lastMouseX = event->pos().x();
    m_lastMouseY = event->pos().y();

    if (event->button() == Qt::LeftButton) {
        m_isRotating = true;
        m_view->StartRotation(m_lastMouseX, m_lastMouseY);
    } else if (event->button() == Qt::MiddleButton || (event->button() == Qt::RightButton && event->modifiers() & Qt::ShiftModifier)) {
        m_isPanning = true;
    }
#endif
}

void OcctModelViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);

#if PYRAQT_HAS_OCCT
    if (!isViewerReady()) {
        return;
    }

    const QPoint pos = event->pos();
    if (m_isRotating) {
        m_view->Rotation(pos.x(), pos.y());
        m_view->Redraw();
    } else if (m_isPanning) {
        m_view->Pan(pos.x() - m_lastMouseX, m_lastMouseY - pos.y(), 1.0, Standard_True);
        m_view->Redraw();
        m_lastMouseX = pos.x();
        m_lastMouseY = pos.y();
    } else {
        updateHoverInfo(pos.x(), pos.y());
    }
#else
    Q_UNUSED(event);
#endif
}

void OcctModelViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);

#if PYRAQT_HAS_OCCT
    if (!isViewerReady()) {
        return;
    }

    const QPoint pos = event->pos();
    if (event->button() == Qt::LeftButton && !m_isPanning) {
        updateHoverInfo(pos.x(), pos.y());
        m_context->SelectDetected(AIS_SelectionScheme_Replace);
        if (!m_view.IsNull()) {
            m_view->Redraw();
        }
        refreshSelectionInfo();
    }

    if (event->button() == Qt::LeftButton) {
        m_isRotating = false;
    }
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_isPanning = false;
    }
#else
    Q_UNUSED(event);
#endif
}

void OcctModelViewWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    if (event->button() == Qt::LeftButton) {
        fitAll();
    }
}

void OcctModelViewWidget::wheelEvent(QWheelEvent *event)
{
    QWidget::wheelEvent(event);

#if PYRAQT_HAS_OCCT
    if (!isViewerReady()) {
        return;
    }

    const QPoint pos = event->pos();
    const int deltaY = event->angleDelta().y();
    if (deltaY == 0) {
        return;
    }

    m_view->StartZoomAtPoint(pos.x(), pos.y());
    m_view->ZoomAtPoint(pos.x(), pos.y(), pos.x(), pos.y() + deltaY / 4);
    m_view->Redraw();
#else
    Q_UNUSED(event);
#endif
}

bool OcctModelViewWidget::isViewerReady() const
{
#if PYRAQT_HAS_OCCT
    return !m_context.IsNull() && !m_view.IsNull();
#else
    return false;
#endif
}

void OcctModelViewWidget::initializeViewer()
{
#if PYRAQT_HAS_OCCT
    if (isViewerReady()) {
        return;
    }

    try {
        m_displayConnection = new Aspect_DisplayConnection();
        m_graphicDriver = new OpenGl_GraphicDriver(m_displayConnection);
        m_viewer = new V3d_Viewer(m_graphicDriver);
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();
        m_viewer->SetDefaultBackgroundColor(Quantity_NOC_GRAY20);

        m_context = new AIS_InteractiveContext(m_viewer);
        m_context->SetAutomaticHilight(Standard_True);
        m_context->SetToHilightSelected(Standard_True);

        m_view = m_viewer->CreateView();
        m_view->SetBackgroundColor(Quantity_NOC_GRAY20);
        m_view->TriedronDisplay(Aspect_TOTP_RIGHT_LOWER, Quantity_NOC_WHITE, 0.08, V3d_WIREFRAME);

        attachViewToNativeWindow();
        applyDisplayMode();
        applySelectionMode();
    } catch (...) {
        m_document.isValid = false;
        if (m_document.summary.errorMessage.isEmpty()) {
            m_document.summary.errorMessage = tr("Failed to initialize OCCT 3D viewer.");
        }
        m_document.statusMessage = m_document.summary.errorMessage;
    }
#endif
}

void OcctModelViewWidget::attachViewToNativeWindow()
{
#if PYRAQT_HAS_OCCT
    if (m_view.IsNull()) {
        return;
    }

    Handle(Aspect_NeutralWindow) neutralWindow = new Aspect_NeutralWindow();
    neutralWindow->SetNativeHandle(static_cast<Aspect_Drawable>(winId()));
    neutralWindow->SetSize(width(), height());
    neutralWindow->Map();
    m_window = neutralWindow;
    m_view->SetWindow(m_window);
    if (!m_window->IsMapped()) {
        m_window->Map();
    }
#endif
}

void OcctModelViewWidget::displayDocument()
{
#if PYRAQT_HAS_OCCT
    if (!isViewerReady()) {
        return;
    }

    m_context->RemoveAll(Standard_False);
    m_context->ClearSelected(Standard_False);
    m_context->ClearDetected(Standard_False);

    if (m_document.isValid && m_document.occtData && m_document.occtData->aisShape) {
        m_context->Display(m_document.occtData->aisShape, Standard_False);
        applyDisplayMode();
        applySelectionMode();
        fitAll();
    } else {
        m_view->Redraw();
    }
#endif
}

void OcctModelViewWidget::applyDisplayMode()
{
#if PYRAQT_HAS_OCCT
    if (m_context.IsNull() || !m_document.occtData || m_document.occtData->aisShape.IsNull()) {
        return;
    }

    const int mode = m_document.displayMode == pyraqt::core::ModelDisplayMode::Wireframe ? AIS_WireFrame : AIS_Shaded;
    m_context->SetDisplayMode(m_document.occtData->aisShape, mode, Standard_False);

    Handle(Prs3d_Drawer) attributes = m_document.occtData->aisShape->Attributes();
    attributes->SetFaceBoundaryDraw(m_document.displayMode == pyraqt::core::ModelDisplayMode::ShadedWithEdges);
    attributes->SetIsoOnTriangulation(m_document.displayMode == pyraqt::core::ModelDisplayMode::ShadedWithEdges);
    m_context->Redisplay(m_document.occtData->aisShape, Standard_True);
#endif
}

void OcctModelViewWidget::applySelectionMode()
{
#if PYRAQT_HAS_OCCT
    if (m_context.IsNull() || !m_document.occtData || m_document.occtData->aisShape.IsNull()) {
        return;
    }

    m_context->Deactivate(m_document.occtData->aisShape);
    m_context->Activate(m_document.occtData->aisShape, occtSelectionMode(m_document.selectionMode), Standard_True);
    m_context->UpdateCurrentViewer();
#endif
}

void OcctModelViewWidget::updateHoverInfo(const int x, const int y)
{
#if PYRAQT_HAS_OCCT
    if (m_context.IsNull() || m_view.IsNull()) {
        return;
    }

    m_context->MoveTo(x, y, m_view, Standard_True);
    emit hoverInfoChanged(buildSelectionInfo(false));
#else
    Q_UNUSED(x);
    Q_UNUSED(y);
#endif
}

void OcctModelViewWidget::refreshSelectionInfo()
{
    m_selectionInfo = buildSelectionInfo(true);
    emit selectionInfoChanged(m_selectionInfo);
}

pyraqt::core::ModelSelectionInfo OcctModelViewWidget::buildSelectionInfo(const bool selected) const
{
    pyraqt::core::ModelSelectionInfo info;

#if PYRAQT_HAS_OCCT
    if (m_context.IsNull()) {
        return info;
    }

    const bool hasShape = selected ? m_context->HasSelectedShape() : m_context->HasDetectedShape();
    if (!hasShape) {
        return info;
    }

    const TopoDS_Shape shape = selected ? m_context->SelectedShape() : m_context->DetectedShape();
    if (shape.IsNull()) {
        return info;
    }

    info.hasSelection = true;
    info.kind = selectionKindFromShape(shape);
    info.label = shapeLabel(shape);
    info.measurements = measureShape(shape);
    info.measureText = pyraqt::core::ModelPropertyService::formatMeasureSummary(info.measurements);

    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (!box.IsVoid()) {
        Standard_Real xmin = 0.0;
        Standard_Real ymin = 0.0;
        Standard_Real zmin = 0.0;
        Standard_Real xmax = 0.0;
        Standard_Real ymax = 0.0;
        Standard_Real zmax = 0.0;
        box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        info.boundingBoxText = boundsTextFromMinMax(xmin, ymin, zmin, xmax, ymax, zmax);
    }
#else
    Q_UNUSED(selected);
#endif

    return info;
}

} // namespace pyraqt::ui
