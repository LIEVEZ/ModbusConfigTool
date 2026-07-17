#include "runtime_value_table_model.h"

#include <QBrush>

RuntimeValueTableModel::RuntimeValueTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void RuntimeValueTableModel::setDocument(const ProjectDocument *document)
{
    beginResetModel(); m_document = document; endResetModel();
}

int RuntimeValueTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() || !m_document ? 0 : m_document->registers.size();
}

int RuntimeValueTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 7;
}

QVariant RuntimeValueTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_document || index.row() >= m_document->registers.size()) { return QVariant(); }
    const RegisterPoint &point = m_document->registers.at(index.row());
    if (role == Qt::ForegroundRole && !point.enabled) { return QBrush(QColor(QStringLiteral("#8a8983"))); }
    if (role != Qt::DisplayRole) { return QVariant(); }
    switch (index.column())
    {
    case 0: return groupName(point.groupId);
    case 1: return point.slaveAddress;
    case 2: return point.address;
    case 3: return point.name;
    case 4: return point.currentValue.toDisplayString(point.precision);
    case 5: return dataTypeToString(point.dataType);
    case 6: return point.enabled ? QStringLiteral("是") : QStringLiteral("否");
    default: return QVariant();
    }
}

QVariant RuntimeValueTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) { return QVariant(); }
    static const QStringList headers = {QStringLiteral("分组"), QStringLiteral("从站"),
        QStringLiteral("地址"), QStringLiteral("名称"), QStringLiteral("当前值"),
        QStringLiteral("类型"), QStringLiteral("启用")};
    return headers.value(section);
}

QString RuntimeValueTableModel::pointId(int row) const
{
    return m_document && row >= 0 && row < m_document->registers.size()
        ? m_document->registers.at(row).id : QString();
}

void RuntimeValueTableModel::refreshPoints(const QSet<QString> &pointIds)
{
    if (!m_document || pointIds.isEmpty()) { return; }
    for (int row = 0; row < m_document->registers.size(); ++row)
    {
        if (pointIds.contains(m_document->registers.at(row).id))
        {
            emit dataChanged(index(row, 4), index(row, 4), {Qt::DisplayRole});
        }
    }
}

QString RuntimeValueTableModel::groupName(const QString &groupId) const
{
    if (!m_document) { return QString(); }
    for (const RegisterGroup &group : m_document->groups) { if (group.id == groupId) { return group.name; } }
    return QString();
}
