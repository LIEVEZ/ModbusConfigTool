#include "group_panel_view.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

GroupPanelView::GroupPanelView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(this);
    auto *titleRow = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("寄存器分组"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    m_countBadge = new QLabel(this);
    titleRow->addWidget(title, 1); titleRow->addWidget(m_countBadge);
    auto *buttons = new QHBoxLayout;
    auto *addButton = new QPushButton(QStringLiteral("新增分组"), this);
    m_removeButton = new QPushButton(QStringLiteral("删除分组"), this);
    buttons->addWidget(addButton); buttons->addWidget(m_removeButton);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({QStringLiteral("分组"), QStringLiteral("数量")});
    auto *batchButton = new QPushButton(QStringLiteral("批量编辑"), this);
    batchButton->setObjectName(QStringLiteral("primaryButton"));
    layout->addLayout(titleRow); layout->addLayout(buttons); layout->addWidget(m_tree, 1); layout->addWidget(batchButton);
    connect(addButton, &QPushButton::clicked, this, &GroupPanelView::addRequested);
    connect(m_removeButton, &QPushButton::clicked, this, &GroupPanelView::removeRequested);
    connect(batchButton, &QPushButton::clicked, this, &GroupPanelView::batchEditRequested);
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item)
    {
        if (!item) { return; }
        m_removeButton->setEnabled(!item->data(0, Qt::UserRole + 1).toBool());
        emit groupSelected(item->data(0, Qt::UserRole).toString(), item->text(0));
    });
}

void GroupPanelView::setGroups(const QList<RegisterGroup> &groups,
                               const QList<RegisterPoint> &points)
{
    const QString selected = selectedGroupId();
    m_tree->clear();
    auto *all = new QTreeWidgetItem({QStringLiteral("全部分组"), QString::number(points.size())});
    all->setData(0, Qt::UserRole, QString());
    all->setData(0, Qt::UserRole + 1, true);
    m_tree->addTopLevelItem(all);
    for (const RegisterGroup &group : groups)
    {
        int count = 0;
        for (const RegisterPoint &point : points) { if (point.groupId == group.id) { ++count; } }
        auto *item = new QTreeWidgetItem({group.name, QString::number(count)});
        item->setData(0, Qt::UserRole, group.id);
        item->setData(0, Qt::UserRole + 1, group.isDefault);
        m_tree->addTopLevelItem(item);
        if (group.id == selected) { m_tree->setCurrentItem(item); }
    }
    if (!m_tree->currentItem()) { m_tree->setCurrentItem(all); }
    m_countBadge->setText(QStringLiteral("%1 分组").arg(groups.size()));
}

QString GroupPanelView::selectedGroupId() const
{
    return m_tree->currentItem() ? m_tree->currentItem()->data(0, Qt::UserRole).toString() : QString();
}
