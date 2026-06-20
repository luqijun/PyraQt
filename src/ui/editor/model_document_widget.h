#pragma once

#include "core/modeling/model_types.h"

#include <QWidget>

namespace pyraqt::ui {

class ModelSummaryWidget;
class OcctModelViewWidget;

class ModelDocumentWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ModelDocumentWidget(QWidget *parent = nullptr);

    void setDocument(const pyraqt::core::ModelDocument &document);
    void setDisplayMode(pyraqt::core::ModelDisplayMode mode);
    void setSelectionMode(pyraqt::core::ModelSelectionMode mode);
    void fitAll();
    void setStandardView(const QString &viewName);
    void clearSelection();

    [[nodiscard]] pyraqt::core::ModelDocument document() const;
    [[nodiscard]] pyraqt::core::ModelSelectionInfo selectionInfo() const;
    [[nodiscard]] OcctModelViewWidget *viewWidget() const;

signals:
    void displayModeChanged(pyraqt::core::ModelDisplayMode mode);
    void selectionModeChanged(pyraqt::core::ModelSelectionMode mode);
    void selectionInfoChanged(const pyraqt::core::ModelSelectionInfo &selection);
    void hoverInfoChanged(const pyraqt::core::ModelSelectionInfo &selection);
    void statusMessageChanged(const QString &message);

private:
    ModelSummaryWidget *m_errorSummaryWidget = nullptr;
    OcctModelViewWidget *m_viewWidget = nullptr;
    pyraqt::core::ModelDocument m_document;
};

} // namespace pyraqt::ui
