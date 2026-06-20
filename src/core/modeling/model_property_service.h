#pragma once

#include "core/modeling/model_types.h"

namespace pyraqt::core {

class ModelPropertyService final {
public:
    [[nodiscard]] static QString formatDisplayMode(ModelDisplayMode mode);
    [[nodiscard]] static QString formatSelectionMode(ModelSelectionMode mode);
    [[nodiscard]] static QString formatSelectionType(ModelSelectionKind kind);
    [[nodiscard]] static QString formatModelFormat(ModelFormat format);
    [[nodiscard]] static QString formatMeasureSummary(const ModelMeasurementSummary &measurements);
};

} // namespace pyraqt::core
