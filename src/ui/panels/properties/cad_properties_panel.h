#pragma once

#include "core/cad/cad_types.h"

#include <QWidget>

class QEvent;
class QLabel;
class QFormLayout;

namespace pyraqt::ui {

class CadPropertiesPanel final : public QWidget {
    Q_OBJECT

public:
    explicit CadPropertiesPanel(QWidget *parent = nullptr);

    void setCadDocument(const pyraqt::core::CadDocument &document);
    void setSelectionInfo(const pyraqt::core::CadSelectionInfo &selection);
    void clearSelection();
    void showPlaceholder(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();
    void setValue(QLabel *label, const QString &value);
    void updateDisplayedValue(QLabel *label);
    QLabel *createValueLabel(const QString &objectName);

    QFormLayout *m_form = nullptr;
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
