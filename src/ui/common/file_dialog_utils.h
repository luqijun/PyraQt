#pragma once

#include <QFileDialog>
#include <QString>
#include <QStringList>

class QWidget;

namespace pyraqt::ui {

struct FileDialogRequest {
    QString title;
    QString initialPath;
    QString filter;
    QString defaultSuffix;
    QFileDialog::FileMode fileMode = QFileDialog::ExistingFile;
    QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen;
};

QFileDialog *createThemedFileDialog(const FileDialogRequest &request, QWidget *parent = nullptr);
QString getThemedOpenFileName(const FileDialogRequest &request, QWidget *parent = nullptr);
QString getThemedSaveFileName(const FileDialogRequest &request, QWidget *parent = nullptr);
QString getThemedExistingDirectory(const FileDialogRequest &request, QWidget *parent = nullptr);

} // namespace pyraqt::ui
