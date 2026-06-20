#pragma once

#include "core/modeling/model_types.h"

#include <QWidget>

class QLabel;
class QPlainTextEdit;

namespace pyraqt::ui {

class ModelSummaryWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ModelSummaryWidget(QWidget *parent = nullptr);

    void setSummary(const pyraqt::core::ModelImportSummary &summary);
    [[nodiscard]] pyraqt::core::ModelImportSummary summary() const;

private:
    pyraqt::core::ModelImportSummary m_summary;
    QLabel *m_fileValueLabel = nullptr;
    QLabel *m_formatValueLabel = nullptr;
    QLabel *m_statusValueLabel = nullptr;
    QLabel *m_rootShapesValueLabel = nullptr;
    QLabel *m_solidsValueLabel = nullptr;
    QLabel *m_shellsValueLabel = nullptr;
    QLabel *m_facesValueLabel = nullptr;
    QLabel *m_edgesValueLabel = nullptr;
    QLabel *m_verticesValueLabel = nullptr;
    QPlainTextEdit *m_errorViewer = nullptr;
};

} // namespace pyraqt::ui
