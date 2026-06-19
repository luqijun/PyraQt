#pragma once

#include <QWidget>

class QFileSystemModel;
class QModelIndex;
class QTreeView;

namespace pyraqt::ui {

class FileBrowserPanel final : public QWidget {
    Q_OBJECT

public:
    explicit FileBrowserPanel(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    [[nodiscard]] QString rootPath() const;

signals:
    void fileActivated(const QString &path);

private:
    QFileSystemModel *m_model = nullptr;
    QTreeView *m_treeView = nullptr;
};

} // namespace pyraqt::ui
