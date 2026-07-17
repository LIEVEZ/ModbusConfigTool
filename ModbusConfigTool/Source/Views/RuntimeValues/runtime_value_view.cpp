#include "runtime_value_view.h"

#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QTimer>

RuntimeValueView::RuntimeValueView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("实时数值"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    m_model = new RuntimeValueTableModel(this);
    m_proxy = new RegisterFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_table = new QTableView(this);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    layout->addWidget(title); layout->addWidget(m_table, 1);
    connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &index)
    {
        emit locateRequested(m_model->pointId(m_proxy->mapToSource(index).row()));
    });
    auto *refreshTimer = new QTimer(this);
    refreshTimer->setInterval(100);
    connect(refreshTimer, &QTimer::timeout, this, [this]()
    {
        m_model->refreshPoints(m_pendingPointIds);
        m_pendingPointIds.clear();
    });
    refreshTimer->start();
}

void RuntimeValueView::setDocument(const ProjectDocument *document) { m_model->setDocument(document); }
void RuntimeValueView::setGroupFilter(const QString &groupName) { m_proxy->setGroupName(groupName); }

void RuntimeValueView::queuePointRefresh(const QString &pointId)
{
    m_pendingPointIds.insert(pointId);
}
