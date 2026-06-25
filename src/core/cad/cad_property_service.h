#pragma once

#include "core/cad/cad_types.h"

namespace pyraqt::core {

class CadPropertyService final {
public:
    [[nodiscard]] static QString formatDisplayMode(CadDisplayMode mode);
    [[nodiscard]] static QString formatSelectionMode(CadSelectionMode mode);
    [[nodiscard]] static QString formatSelectionType(CadSelectionKind kind);
    [[nodiscard]] static QString formatFormat(CadFormat format);
    [[nodiscard]] static QString formatMeasureSummary(const CadMeasurementSummary &measurements);
};

} // namespace pyraqt::core
