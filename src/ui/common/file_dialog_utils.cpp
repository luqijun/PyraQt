#include "ui/common/file_dialog_utils.h"

#include <QDialog>
#include <memory>

namespace pyraqt::ui {

QFileDialog *createThemedFileDialog(const FileDialogRequest &request, QWidget *parent)
{
    auto *dialog = new QFileDialog(parent, request.title, request.initialPath, request.filter);
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);
    dialog->setFileMode(request.fileMode);
    dialog->setAcceptMode(request.acceptMode);
    if (!request.defaultSuffix.isEmpty()) {
        dialog->setDefaultSuffix(request.defaultSuffix);
    }
    if (!request.filter.isEmpty()) {
        dialog->selectNameFilter(request.filter.split(QStringLiteral(";;")).constFirst());
    }
    return dialog;
}

QString getThemedOpenFileName(const FileDialogRequest &request, QWidget *parent)
{
    std::unique_ptr<QFileDialog> dialog(createThemedFileDialog(request, parent));
    if (dialog->exec() != QDialog::Accepted) {
        return {};
    }

    const QStringList files = dialog->selectedFiles();
    return files.isEmpty() ? QString() : files.constFirst();
}

QString getThemedSaveFileName(const FileDialogRequest &request, QWidget *parent)
{
    std::unique_ptr<QFileDialog> dialog(createThemedFileDialog(request, parent));
    if (dialog->exec() != QDialog::Accepted) {
        return {};
    }

    const QStringList files = dialog->selectedFiles();
    return files.isEmpty() ? QString() : files.constFirst();
}

QString getThemedExistingDirectory(const FileDialogRequest &request, QWidget *parent)
{
    std::unique_ptr<QFileDialog> dialog(createThemedFileDialog(request, parent));
    dialog->setOption(QFileDialog::ShowDirsOnly, true);
    if (dialog->exec() != QDialog::Accepted) {
        return {};
    }

    const QStringList files = dialog->selectedFiles();
    return files.isEmpty() ? QString() : files.constFirst();
}

} // namespace pyraqt::ui
