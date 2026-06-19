#pragma once

#include <QDialog>

class QListWidget;
class QLineEdit;

namespace pyraqt::core {
class CommandManager;
}

namespace pyraqt::ui {

class CommandPaletteDialog final : public QDialog {
    Q_OBJECT

public:
    explicit CommandPaletteDialog(pyraqt::core::CommandManager &commandManager, QWidget *parent = nullptr);

private:
    void refreshList();

    pyraqt::core::CommandManager &m_commandManager;
    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_resultsList = nullptr;
};

} // namespace pyraqt::ui
