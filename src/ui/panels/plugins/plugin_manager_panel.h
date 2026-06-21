#pragma once

#include <QWidget>

class QTableWidget;

namespace pyraqt::core {
class PluginManager;
}

namespace pyraqt::ui {

class PluginManagerPanel final : public QWidget {
    Q_OBJECT

public:
    explicit PluginManagerPanel(pyraqt::core::PluginManager &pluginManager, QWidget *parent = nullptr);
    [[nodiscard]] QTableWidget *tableForTesting() const;

private:
    void refreshTable();

    pyraqt::core::PluginManager &m_pluginManager;
    QTableWidget *m_table = nullptr;
};

} // namespace pyraqt::ui
