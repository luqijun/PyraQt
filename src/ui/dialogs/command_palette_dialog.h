#pragma once

#include <QDialog>

class QEvent;
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

protected:
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();
    void refreshList();

    pyraqt::core::CommandManager &m_commandManager;
    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_resultsList = nullptr;
};

} // namespace pyraqt::ui
