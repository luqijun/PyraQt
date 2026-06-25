#pragma once

#include "core/cad/cad_types.h"

#include <QObject>

namespace pyraqt::core {

class CadImportManager final : public QObject {
    Q_OBJECT

public:
    explicit CadImportManager(QObject *parent = nullptr);

    [[nodiscard]] bool isOcctAvailable() const;
    [[nodiscard]] bool isSupportedFile(const QString &path) const;
    [[nodiscard]] CadFormat detectFormat(const QString &path) const;
    [[nodiscard]] CadDocument importFile(const QString &path) const;
};

} // namespace pyraqt::core
