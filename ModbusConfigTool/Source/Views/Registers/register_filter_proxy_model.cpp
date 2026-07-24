#include "register_filter_proxy_model.h"

RegisterFilterProxyModel::RegisterFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void RegisterFilterProxyModel::setSearchMode(SearchMode mode) { m_mode = mode; invalidateFilter(); }
void RegisterFilterProxyModel::setSearchText(const QString &text) { m_text = text.trimmed(); invalidateFilter(); }
void RegisterFilterProxyModel::setGroupName(const QString &name) { m_groupName = name; invalidateFilter(); }

bool RegisterFilterProxyModel::filterAcceptsRow(int sourceRow,
                                                const QModelIndex &sourceParent) const
{
    const auto value = [this, sourceRow, sourceParent](int column)
    {
        return sourceModel()->index(sourceRow, column, sourceParent).data().toString();
    };
    if (!m_groupName.isEmpty() && value(0) != m_groupName) { return false; }
    if (m_text.isEmpty()) { return true; }
    QList<int> columns;
    switch (m_mode)
    {
    case SearchMode::Name: columns = {4}; break;
    case SearchMode::Address: columns = {2}; break;
    case SearchMode::Category: columns = {15}; break;
    case SearchMode::ProtocolKey: columns = {14}; break;
    case SearchMode::All: columns = {0, 1, 2, 4, 5, 6, 9, 10, 12, 14, 15, 16, 18}; break;
    default: break;
    }
    for (int column : columns)
    {
        if (value(column).contains(m_text, Qt::CaseInsensitive)) { return true; }
    }
    return false;
}
