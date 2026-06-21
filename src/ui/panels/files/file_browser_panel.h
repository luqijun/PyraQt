#pragma once

#include <QWidget>

class QEvent;
class QFileSystemModel;
class QModelIndex;
class QPoint;
class QTreeView;

namespace pyraqt::ui {

class FileBrowserPanel final : public QWidget {
    Q_OBJECT

public:
    explicit FileBrowserPanel(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    [[nodiscard]] QString rootPath() const;
    [[nodiscard]] QString toolTipForIndex(const QModelIndex &index) const;

signals:
    void fileActivated(const QString &path);
    void renameRequested(const QString &path);
    void deleteRequested(const QString &path);

protected:
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();
    void showContextMenu(const QPoint &position);

    QFileSystemModel *m_model = nullptr;
    QTreeView *m_treeView = nullptr;
};

} // namespace pyraqt::ui
