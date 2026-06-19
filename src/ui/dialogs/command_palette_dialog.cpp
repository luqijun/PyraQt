#include "ui/dialogs/command_palette_dialog.h"

#include "core/command/command_manager.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace pyraqt::ui {

CommandPaletteDialog::CommandPaletteDialog(pyraqt::core::CommandManager &commandManager, QWidget *parent)
    : QDialog(parent)
    , m_commandManager(commandManager)
{
    setWindowTitle(tr("Command Palette"));
    resize(560, 420);

    auto *layout = new QVBoxLayout(this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search commands..."));
    m_resultsList = new QListWidget(this);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);

    layout->addWidget(m_searchEdit);
    layout->addWidget(m_resultsList);
    layout->addWidget(buttonBox);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] {
        refreshList();
    });
    connect(m_resultsList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        if (item == nullptr) {
            return;
        }
        const QString commandId = item->data(Qt::UserRole).toString();
        const bool executed = m_commandManager.execute(commandId);
        Q_UNUSED(executed);
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(&m_commandManager, &pyraqt::core::CommandManager::commandsChanged, this, [this] {
        refreshList();
    });

    refreshList();
}

void CommandPaletteDialog::refreshList()
{
    m_resultsList->clear();
    const QString needle = m_searchEdit->text().trimmed();

    for (const auto &command : m_commandManager.commands()) {
        const QString haystack = QStringLiteral("%1 %2 %3 %4")
                                     .arg(command.title, command.description, command.source, command.keywords.join(' '));
        if (!needle.isEmpty() && !haystack.contains(needle, Qt::CaseInsensitive)) {
            continue;
        }

        auto *item = new QListWidgetItem(
            QStringLiteral("%1 [%2]").arg(command.title, command.source.isEmpty() ? tr("Built-in") : command.source),
            m_resultsList);
        item->setData(Qt::UserRole, command.id);
        item->setToolTip(command.description);
        if (!command.enabled) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }
}

} // namespace pyraqt::ui
