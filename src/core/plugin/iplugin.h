#pragma once

#include <QtPlugin>

class QString;

namespace pyraqt::core {

class PluginContext;

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual bool initialize(PluginContext &context) = 0;
    virtual void shutdown() = 0;
};

} // namespace pyraqt::core

#define PYRAQT_IPLUGIN_IID "org.pyraqt.IPlugin/1.0"
Q_DECLARE_INTERFACE(pyraqt::core::IPlugin, PYRAQT_IPLUGIN_IID)
