import pyra


class AnalysisToolsPlugin:
    def __init__(self, iface):
        self.iface = iface

    def initGui(self):
        pyra.log.info("Analysis Tools Plugin initialized")
        pyra.expressions.register(
            "slugify_text",
            "def slugify_text(value):\n"
            "    return str(value).strip().lower().replace(' ', '-')\n",
        )
        pyra.processing.register(
            "batch_label",
            "def batch_label(params):\n"
            "    name = str(params.get('name', 'job')).strip()\n"
            "    count = str(params.get('count', '1')).strip()\n"
            "    return f'{name}-x{count}'\n",
        )
        pyra.commands.register(
            "analysis_tools_plugin.preview",
            self.preview,
            title="Analysis Tools Plugin: Preview Helpers",
            description="Preview the expression and processing helpers shipped by this plugin.",
            keywords=["plugin", "expression", "processing"],
        )

    def unload(self):
        pyra.log.info("Analysis Tools Plugin unloaded")

    def preview(self):
        pyra.log.info("Expression helper: slugify_text")
        pyra.log.info("Processing helper: batch_label")
        pyra.ui.set_status("Analysis helpers are ready")


def classFactory(iface):
    return AnalysisToolsPlugin(iface)
