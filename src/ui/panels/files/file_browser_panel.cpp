#include "ui/panels/files/file_browser_panel.h"

#include <QEvent>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

namespace pyraqt::ui {

namespace {

class FileBrowserFileSystemModel final : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::ToolTipRole && index.isValid()) {
            return QFileSystemModel::data(index, QFileSystemModel::FilePathRole);
        }
        return QFileSystemModel::data(index, role);
    }
};

}

FileBrowserPanel::FileBrowserPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("fileBrowserPanel"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_model = new FileBrowserFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    m_model->setRootPath(QDir::homePath());

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setRootIndex(m_model->index(QDir::homePath()));
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setMouseTracking(true);
    m_treeView->header()->setStretchLastSection(true);
    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const QString path = m_model->filePath(index);
        if (QFileInfo(path).isFile()) {
            emit fileActivated(path);
        }
    });
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileBrowserPanel::showContextMenu);
    retranslateUi();
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

QString FileBrowserPanel::toolTipForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }
    return index.data(QFileSystemModel::FilePathRole).toString();
}

void FileBrowserPanel::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void FileBrowserPanel::retranslateUi()
{
    if (m_treeView != nullptr) {
        m_treeView->setAccessibleName(tr("File Browser"));
        m_treeView->setAccessibleDescription(tr("Local file browser for opening Python scripts"));
    }
}

void FileBrowserPanel::showContextMenu(const QPoint &position)
{
    const QModelIndex index = m_treeView->indexAt(position);
    if (!index.isValid()) {
        return;
    }

    const QString path = m_model->filePath(index);
    if (path.isEmpty()) {
        return;
    }

    QMenu menu(this);
    QAction *renameAction = menu.addAction(tr("Rename"));
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *selectedAction = menu.exec(m_treeView->viewport()->mapToGlobal(position));
    if (selectedAction == renameAction) {
        emit renameRequested(path);
    } else if (selectedAction == deleteAction) {
        emit deleteRequested(path);
    }
}

} // namespace pyraqt::ui
