#pragma once

#include <QColor>
#include <QIcon>
#include <QSize>
#include <QString>

namespace pyraqt::ui {

[[nodiscard]] QColor iconColorForTheme(const QString &themeName);
[[nodiscard]] QIcon themedSvgIcon(const QString &iconName, const QString &themeName, const QSize &size = QSize(24, 24));

} // namespace pyraqt::ui
