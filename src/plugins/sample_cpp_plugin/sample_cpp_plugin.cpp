#include "plugins/sample_cpp_plugin/sample_cpp_plugin.h"

#include "core/command/command_descriptor.h"
#include "core/command/command_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/plugin_context.h"

namespace pyraqt::plugins {

QString SampleCppPlugin::id() const
{
    return QStringLiteral("sample_cpp_plugin");
}

QString SampleCppPlugin::name() const
{
    return QStringLiteral("Sample C++ Plugin");
}

QString SampleCppPlugin::version() const
{
    return QStringLiteral("0.1.0");
}

bool SampleCppPlugin::initialize(pyraqt::core::PluginContext &context)
{
    pyraqt::core::CommandDescriptor descriptor;
    descriptor.id = QStringLiteral("sample_cpp_plugin.say_hello");
    descriptor.ownerId = id();
    descriptor.title = QStringLiteral("Sample C++ Plugin: Say Hello");
    descriptor.description = QStringLiteral("Write a greeting from the sample C++ plugin.");
    descriptor.source = name();
    descriptor.keywords = QStringList{
        QStringLiteral("plugin"),
        QStringLiteral("hello"),
        QStringLiteral("cpp"),
    };
    descriptor.handler = [&context] {
        context.logManager().info(QStringLiteral("Hello from Sample C++ Plugin"));
    };

    return context.commandManager().registerCommand(descriptor);
}

void SampleCppPlugin::shutdown()
{
}

} // namespace pyraqt::plugins
