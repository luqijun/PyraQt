import pyra


class DependentWorkflowPlugin:
    def __init__(self, iface):
        self.iface = iface

    def initGui(self):
        pyra.log.info("Dependent Workflow Plugin initialized")
        pyra.commands.register(
            "dependent_workflow_plugin.open_runtime_command",
            self.open_runtime_command,
            title="Dependent Workflow Plugin: Trigger Runtime Command",
            description="Trigger the command exposed by runtime_command_plugin.",
            keywords=["plugin", "dependency", "command"],
        )

    def unload(self):
        pyra.log.info("Dependent Workflow Plugin unloaded")

    def open_runtime_command(self):
        pyra.commands.run("runtime_command_plugin.announce")
        pyra.ui.set_status("Dependent workflow triggered runtime command")


def classFactory(iface):
    return DependentWorkflowPlugin(iface)
