import pyra


class RuntimeCommandPlugin:
    def __init__(self, iface):
        self.iface = iface

    def initGui(self):
        pyra.log.info("Runtime Command Plugin initialized")
        pyra.ui.set_status("Runtime Command Plugin loaded")
        pyra.commands.register(
            "runtime_command_plugin.announce",
            self.announce,
            title="Runtime Command Plugin: Announce",
            description="Announce plugin state through the shared Python runtime.",
            keywords=["plugin", "runtime", "status"],
        )

    def unload(self):
        pyra.log.info("Runtime Command Plugin unloaded")

    def announce(self):
        pyra.log.info("Runtime Command Plugin command executed")
        pyra.ui.show_message("Runtime Command Plugin says hello")


def classFactory(iface):
    return RuntimeCommandPlugin(iface)
