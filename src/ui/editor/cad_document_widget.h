#pragma once

#include "core/cad/cad_types.h"

#include <QWidget>

namespace pyraqt::ui {

class CadSummaryWidget;
class OcctCadViewWidget;

class CadDocumentWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CadDocumentWidget(QWidget *parent = nullptr);

    void setDocument(const pyraqt::core::CadDocument &document);
    void setDocumentFilePath(const QString &filePath);
    void setDisplayMode(pyraqt::core::CadDisplayMode mode);
    void setSelectionMode(pyraqt::core::CadSelectionMode mode);
    void fitAll();
    void setStandardView(const QString &viewKey);
    void clearSelection();

    [[nodiscard]] pyraqt::core::CadDocument document() const;
    [[nodiscard]] pyraqt::core::CadSelectionInfo selectionInfo() const;
    [[nodiscard]] OcctCadViewWidget *viewWidget() const;

signals:
    void displayModeChanged(pyraqt::core::CadDisplayMode mode);
    void selectionModeChanged(pyraqt::core::CadSelectionMode mode);
    void selectionInfoChanged(const pyraqt::core::CadSelectionInfo &selection);
    void hoverInfoChanged(const pyraqt::core::CadSelectionInfo &selection);
    void statusMessageChanged(const QString &message);

private:
    CadSummaryWidget *m_errorSummaryWidget = nullptr;
    OcctCadViewWidget *m_viewWidget = nullptr;
    pyraqt::core::CadDocument m_document;
};

} // namespace pyraqt::ui
