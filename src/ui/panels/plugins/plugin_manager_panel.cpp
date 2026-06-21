#include "ui/panels/plugins/plugin_manager_panel.h"

#include "core/plugin/plugin_manager.h"
#include "core/plugin/plugin_types.h"

#include <QEvent>
#include <QCheckBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace pyraqt::ui {

PluginManagerPanel::PluginManagerPanel(pyraqt::core::PluginManager &pluginManager, QWidget *parent)
    : QWidget(parent)
    , m_pluginManager(pluginManager)
{
    setObjectName(QStringLiteral("pluginManagerPanel"));

    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("pluginManagerTable"));
    m_table->setColumnCount(7);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_table);

    connect(&m_pluginManager, &pyraqt::core::PluginManager::pluginsRefreshed, this, [this] {
        refreshTable();
    });

    retranslateUi();
    refreshTable();
}

void PluginManagerPanel::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
        refreshTable();
    }
    QWidget::changeEvent(event);
}

void PluginManagerPanel::retranslateUi()
{
    if (m_table != nullptr) {
        m_table->setHorizontalHeaderLabels({
            tr("Enabled"),
            tr("Name"),
            tr("Type"),
            tr("Version"),
            tr("Status"),
            tr("Actions"),
            tr("Description"),
        });
    }
}

void PluginManagerPanel::refreshTable()
{
    const auto plugins = m_pluginManager.plugins();
    m_table->clearContents();
    m_table->setRowCount(plugins.size());

    for (int row = 0; row < plugins.size(); ++row) {
        const auto &plugin = plugins.at(row);

        auto *checkBox = new QCheckBox(m_table);
        checkBox->setChecked(plugin.enabled);
        checkBox->setToolTip(plugin.name);
        connect(checkBox, &QCheckBox::toggled, this, [this, plugin](bool checked) {
            m_pluginManager.setPluginEnabled(plugin.id, checked);
        });
        m_table->setCellWidget(row, 0, checkBox);
        auto *nameItem = new QTableWidgetItem(plugin.name);
        nameItem->setToolTip(plugin.name);
        m_table->setItem(row, 1, nameItem);
        auto *typeItem = new QTableWidgetItem(plugin.type);
        typeItem->setToolTip(plugin.type);
        m_table->setItem(row, 2, typeItem);
        auto *versionItem = new QTableWidgetItem(plugin.version);
        versionItem->setToolTip(plugin.version);
        m_table->setItem(row, 3, versionItem);
        QString status = tr("Discovered");
        if (!plugin.enabled) {
            status = tr("Disabled");
        } else if (!plugin.error.isEmpty()) {
            status = tr("Error");
        } else if (plugin.loaded && plugin.active) {
            status = tr("Active");
        } else if (plugin.loaded) {
            status = tr("Loaded");
        }
        auto *statusItem = new QTableWidgetItem(status);
        statusItem->setToolTip(status);
        m_table->setItem(row, 4, statusItem);
        auto *reloadButton = new QPushButton(tr("Reload"), m_table);
        reloadButton->setEnabled(plugin.enabled && (plugin.type == QStringLiteral("python") || plugin.type == QStringLiteral("cpp")));
        reloadButton->setToolTip(tr("Reload %1").arg(plugin.name));
        connect(reloadButton, &QPushButton::clicked, this, [this, plugin] {
            m_pluginManager.reloadPlugin(plugin.id);
        });
        m_table->setCellWidget(row, 5, reloadButton);
        const QString details = plugin.error.isEmpty() ? plugin.description : plugin.error;
        auto *detailsItem = new QTableWidgetItem(details);
        detailsItem->setToolTip(details);
        m_table->setItem(row, 6, detailsItem);
    }
}

QTableWidget *PluginManagerPanel::tableForTesting() const
{
    return m_table;
}

} // namespace pyraqt::ui
