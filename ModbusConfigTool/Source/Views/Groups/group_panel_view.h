#ifndef GROUP_PANEL_VIEW_H
#define GROUP_PANEL_VIEW_H

#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QTreeWidget;

class GroupPanelView : public QWidget
{
    Q_OBJECT

public:
    explicit GroupPanelView(QWidget *parent = nullptr);
    void setGroups(const QList<RegisterGroup> &groups, const QList<RegisterPoint> &points);
    QString selectedGroupId() const;

signals:
    void groupSelected(const QString &groupId, const QString &groupName);
    void addRequested();
    void removeRequested();
    void batchEditRequested();

private:
    QLabel *m_countBadge = nullptr;
    QTreeWidget *m_tree = nullptr;
    QPushButton *m_removeButton = nullptr;
};

#endif
