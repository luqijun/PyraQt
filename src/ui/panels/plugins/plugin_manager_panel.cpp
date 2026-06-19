#include "ui/panels/plugins/plugin_manager_panel.h"

#include "core/plugin/plugin_manager.h"
#include "core/plugin/plugin_types.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace pyraqt::ui {

PluginManagerPanel::PluginManagerPanel(pyraqt::core::PluginManager &pluginManager, QWidget *parent)
    : QWidget(parent)
    , m_pluginManager(pluginManager)
{
    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        tr("Enabled"),
        tr("Name"),
        tr("Type"),
        tr("Version"),
        tr("Status"),
        tr("Description"),
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_table);

    connect(&m_pluginManager, &pyraqt::core::PluginManager::pluginsRefreshed, this, [this] {
        refreshTable();
    });

    refreshTable();
}

void PluginManagerPanel::refreshTable()
{
    const auto plugins = m_pluginManager.plugins();
    m_table->setRowCount(plugins.size());

    for (int row = 0; row < plugins.size(); ++row) {
        const auto &plugin = plugins.at(row);

        auto *checkBox = new QCheckBox(m_table);
        checkBox->setChecked(plugin.enabled);
        connect(checkBox, &QCheckBox::toggled, this, [this, plugin](bool checked) {
            m_pluginManager.setPluginEnabled(plugin.id, checked);
        });
        m_table->setCellWidget(row, 0, checkBox);
        m_table->setItem(row, 1, new QTableWidgetItem(plugin.name));
        m_table->setItem(row, 2, new QTableWidgetItem(plugin.type));
        m_table->setItem(row, 3, new QTableWidgetItem(plugin.version));
        m_table->setItem(row, 4, new QTableWidgetItem(plugin.loaded ? tr("Loaded") : tr("Discovered")));
        m_table->setItem(row, 5, new QTableWidgetItem(plugin.error.isEmpty() ? plugin.description : plugin.error));
    }
}

} // namespace pyraqt::ui
