#pragma once

#include "core/modeling/model_types.h"

#include <QWidget>

class QLabel;
class QFormLayout;

namespace pyraqt::ui {

class ModelPropertiesPanel final : public QWidget {
    Q_OBJECT

public:
    explicit ModelPropertiesPanel(QWidget *parent = nullptr);

    void setModelDocument(const pyraqt::core::ModelDocument &document);
    void setSelectionInfo(const pyraqt::core::ModelSelectionInfo &selection);
    void clearSelection();
    void showPlaceholder(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setValue(QLabel *label, const QString &value);
    void updateDisplayedValue(QLabel *label);
    QLabel *createValueLabel(const QString &objectName);

    QLabel *m_stateLabel = nullptr;
    QLabel *m_fileLabel = nullptr;
    QLabel *m_formatLabel = nullptr;
    QLabel *m_boundsLabel = nullptr;
    QLabel *m_solidsLabel = nullptr;
    QLabel *m_facesLabel = nullptr;
    QLabel *m_edgesLabel = nullptr;
    QLabel *m_measureLabel = nullptr;
    QLabel *m_selectionTypeLabel = nullptr;
    QLabel *m_selectionLabel = nullptr;
    QLabel *m_selectionBoundsLabel = nullptr;
    QLabel *m_selectionMeasureLabel = nullptr;
};

} // namespace pyraqt::ui
