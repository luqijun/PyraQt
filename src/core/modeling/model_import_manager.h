#pragma once

#include "core/modeling/model_types.h"

#include <QObject>

namespace pyraqt::core {

class ModelImportManager final : public QObject {
    Q_OBJECT

public:
    explicit ModelImportManager(QObject *parent = nullptr);

    [[nodiscard]] bool isOcctAvailable() const;
    [[nodiscard]] bool isSupportedFile(const QString &path) const;
    [[nodiscard]] ModelFormat detectFormat(const QString &path) const;
    [[nodiscard]] ModelDocument importFile(const QString &path) const;
};

} // namespace pyraqt::core
