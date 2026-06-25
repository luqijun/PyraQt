#pragma once

#include "core/cad/cad_types.h"

#include <QWidget>

#if PYRAQT_HAS_OCCT
#include <Standard_Handle.hxx>
#endif

class QMouseEvent;
class QWheelEvent;

#if PYRAQT_HAS_OCCT
class AIS_InteractiveContext;
class Aspect_DisplayConnection;
class Aspect_Window;
class OpenGl_GraphicDriver;
class V3d_View;
class V3d_Viewer;
#endif

namespace pyraqt::ui {

class OcctCadViewWidget final : public QWidget {
    Q_OBJECT

public:
    explicit OcctCadViewWidget(QWidget *parent = nullptr);
    ~OcctCadViewWidget() override;

    void setDocument(const pyraqt::core::CadDocument &document);
    void setDisplayMode(pyraqt::core::CadDisplayMode mode);
    void setSelectionMode(pyraqt::core::CadSelectionMode mode);
    void fitAll();
    void setStandardView(const QString &viewKey);
    void clearSelection();

    [[nodiscard]] pyraqt::core::CadDocument document() const;
    [[nodiscard]] pyraqt::core::CadSelectionInfo selectionInfo() const;

signals:
    void displayModeChanged(pyraqt::core::CadDisplayMode mode);
    void selectionModeChanged(pyraqt::core::CadSelectionMode mode);
    void selectionInfoChanged(const pyraqt::core::CadSelectionInfo &selection);
    void hoverInfoChanged(const pyraqt::core::CadSelectionInfo &selection);
    void statusMessageChanged(const QString &message);

private:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    [[nodiscard]] bool isViewerReady() const;
    void initializeViewer();
    void attachViewToNativeWindow();
    void displayDocument();
    void applyDisplayMode();
    void applySelectionMode();
    void updateHoverInfo(int x, int y);
    void refreshSelectionInfo();
    [[nodiscard]] pyraqt::core::CadSelectionInfo buildSelectionInfo(bool selected) const;

    bool m_isRotating = false;
    bool m_isPanning = false;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;

    pyraqt::core::CadDocument m_document;
    pyraqt::core::CadSelectionInfo m_selectionInfo;

#if PYRAQT_HAS_OCCT
    Handle(Aspect_DisplayConnection) m_displayConnection;
    Handle(OpenGl_GraphicDriver) m_graphicDriver;
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(Aspect_Window) m_window;
#endif
};

} // namespace pyraqt::ui
