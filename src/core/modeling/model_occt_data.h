#pragma once

#include "core/modeling/model_types.h"

#if PYRAQT_HAS_OCCT
#include <AIS_Shape.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Shape.hxx>
#endif

namespace pyraqt::core {

struct ModelOcctData {
#if PYRAQT_HAS_OCCT
    TopoDS_Shape rootShape;
    Bnd_Box boundingBox;
    Handle(AIS_Shape) aisShape;
#endif
};

} // namespace pyraqt::core
