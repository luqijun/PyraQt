import pyra

class SamplePythonPlugin:
    def __init__(self, iface):
        self.iface = iface

    def initGui(self):
        pyra.log.info("Sample Python Plugin initialized")
        pyra.ui.set_status("Sample Python Plugin loaded")

    def unload(self):
        pyra.log.info("Sample Python Plugin unloaded")


def classFactory(iface):
    return SamplePythonPlugin(iface)


def say_hello():
    pyra.log.info("Hello from Sample Python Plugin")
    pyra.ui.set_status("Sample Python Plugin command executed")


if __name__ == "__main__":
    say_hello()
