#include "ui/common/icon_utils.h"

#include <QCache>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QSvgRenderer>

namespace pyraqt::ui {
namespace {

QString colorToken(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QString cacheKey(const QString &iconName, const QString &themeName, const QSize &size)
{
    return QStringLiteral("%1|%2|%3x%4").arg(iconName, themeName).arg(size.width()).arg(size.height());
}

QByteArray tintedSvgData(const QString &iconName, const QColor &color)
{
    QFile file(QStringLiteral(":/icons/%1.svg").arg(iconName));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QByteArray svg = file.readAll();
    svg.replace("#000000", colorToken(color).toUtf8());
    return svg;
}

QColor disabledIconColorForTheme(const QString &themeName)
{
    if (themeName == QStringLiteral("light")) {
        return QColor(QStringLiteral("#94a3b8"));
    }
    return QColor(QStringLiteral("#9aa9bf"));
}

QPixmap renderSvgPixmap(const QByteArray &svgData, const QSize &size)
{
    QSvgRenderer renderer(svgData);
    if (!renderer.isValid()) {
        return {};
    }

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    return pixmap;
}

} // namespace

QColor iconColorForTheme(const QString &themeName)
{
    if (themeName == QStringLiteral("light")) {
        return QColor(QStringLiteral("#1f2937"));
    }
    return QColor(QStringLiteral("#e5edf7"));
}

QIcon themedSvgIcon(const QString &iconName, const QString &themeName, const QSize &size)
{
    static QCache<QString, QIcon> cache(256);

    const QString key = cacheKey(iconName, themeName, size);
    if (QIcon *cached = cache.object(key)) {
        return *cached;
    }

    const QByteArray normalSvgData = tintedSvgData(iconName, iconColorForTheme(themeName));
    const QByteArray disabledSvgData = tintedSvgData(iconName, disabledIconColorForTheme(themeName));
    if (normalSvgData.isEmpty() || disabledSvgData.isEmpty()) {
        return {};
    }

    const QPixmap normalPixmap = renderSvgPixmap(normalSvgData, size);
    const QPixmap disabledPixmap = renderSvgPixmap(disabledSvgData, size);
    if (normalPixmap.isNull() || disabledPixmap.isNull()) {
        return {};
    }

    auto *icon = new QIcon();
    icon->addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);
    icon->addPixmap(normalPixmap, QIcon::Active, QIcon::Off);
    icon->addPixmap(normalPixmap, QIcon::Selected, QIcon::Off);
    icon->addPixmap(disabledPixmap, QIcon::Disabled, QIcon::Off);
    cache.insert(key, icon);
    return *icon;
}

} // namespace pyraqt::ui
