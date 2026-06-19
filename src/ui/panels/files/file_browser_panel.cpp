#include "ui/panels/files/file_browser_panel.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

namespace pyraqt::ui {

FileBrowserPanel::FileBrowserPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    m_model->setRootPath(QDir::homePath());

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setRootIndex(m_model->index(QDir::homePath()));
    m_treeView->setAccessibleName(tr("File Browser"));
    m_treeView->setAccessibleDescription(tr("Local file browser for opening Python scripts"));
    m_treeView->header()->setStretchLastSection(true);
    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const QString path = m_model->filePath(index);
        if (QFileInfo(path).isFile()) {
            emit fileActivated(path);
        }
    });
}

void FileBrowserPanel::setRootPath(const QString &path)
{
    const QString root = path.isEmpty() ? QDir::homePath() : path;
    m_treeView->setRootIndex(m_model->setRootPath(root));
}

QString FileBrowserPanel::rootPath() const
{
    return m_model->filePath(m_treeView->rootIndex());
}

} // namespace pyraqt::ui
