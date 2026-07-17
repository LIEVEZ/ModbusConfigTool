#include "register_config_view.h"

#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

RegisterConfigView::RegisterConfigView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("寄存器列表"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    auto *tools = new QHBoxLayout;
    auto *mode = new QComboBox(this);
    mode->addItems({QStringLiteral("全部字段"), QStringLiteral("名称"), QStringLiteral("地址"), QStringLiteral("分类"), QStringLiteral("协议键")});
    auto *search = new QLineEdit(this);
    search->setPlaceholderText(QStringLiteral("搜索名称、地址或协议键..."));
    auto *add = new QPushButton(QStringLiteral("新增寄存器"), this);
    auto *edit = new QPushButton(QStringLiteral("编辑详情"), this);
    auto *remove = new QPushButton(QStringLiteral("删除选中"), this);
    auto *enable = new QPushButton(QStringLiteral("启用"), this);
    auto *disable = new QPushButton(QStringLiteral("停用"), this);
    add->setObjectName(QStringLiteral("primaryButton"));
    remove->setObjectName(QStringLiteral("dangerButton"));
    const QList<QWidget *> toolWidgets = {mode, search, add, edit, remove, enable, disable};
    for (QWidget *widget : toolWidgets) { tools->addWidget(widget); }
    tools->setStretch(1, 1);
    m_model = new RegisterTableModel(this);
    m_proxy = new RegisterFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_table = new QTableView(this);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(title); layout->addLayout(tools); layout->addWidget(m_table, 1);
    connect(search, &QLineEdit::textChanged, m_proxy, &RegisterFilterProxyModel::setSearchText);
    connect(mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
    {
        m_proxy->setSearchMode(static_cast<RegisterFilterProxyModel::SearchMode>(index));
    });
    connect(add, &QPushButton::clicked, this, &RegisterConfigView::addRequested);
    connect(edit, &QPushButton::clicked, this, [this]() { emit editRequested(currentPointId()); });
    connect(remove, &QPushButton::clicked, this, [this]() { emit deleteRequested(selectedPointIds()); });
    connect(enable, &QPushButton::clicked, this, [this]() { emit enableRequested(selectedPointIds(), true); });
    connect(disable, &QPushButton::clicked, this, [this]() { emit enableRequested(selectedPointIds(), false); });
    connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &) { emit editRequested(currentPointId()); });
}

void RegisterConfigView::setDocument(const ProjectDocument *document) { m_model->setDocument(document); }
void RegisterConfigView::setGroupFilter(const QString &groupName) { m_proxy->setGroupName(groupName); }

QStringList RegisterConfigView::selectedPointIds() const
{
    QStringList ids;
    for (const QModelIndex &proxyIndex : m_table->selectionModel()->selectedRows())
    {
        const QString id = m_model->pointId(m_proxy->mapToSource(proxyIndex).row());
        if (!id.isEmpty()) { ids.append(id); }
    }
    return ids;
}

QString RegisterConfigView::currentPointId() const
{
    const QModelIndex index = m_table->currentIndex();
    return index.isValid() ? m_model->pointId(m_proxy->mapToSource(index).row()) : QString();
}
