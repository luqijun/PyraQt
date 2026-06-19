#pragma once

#include "core/plugin/iplugin.h"

#include <QObject>

namespace pyraqt::plugins {

class SampleCppPlugin final : public QObject, public core::IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PYRAQT_IPLUGIN_IID)
    Q_INTERFACES(pyraqt::core::IPlugin)

public:
    QString id() const override;
    QString name() const override;
    QString version() const override;
    bool initialize(pyraqt::core::PluginContext &context) override;
    void shutdown() override;
};

} // namespace pyraqt::plugins
