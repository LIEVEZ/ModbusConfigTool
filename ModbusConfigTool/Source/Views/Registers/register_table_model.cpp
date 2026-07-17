#include "register_table_model.h"

#include <QBrush>

RegisterTableModel::RegisterTableModel(QObject *parent) : QAbstractTableModel(parent) {}

void RegisterTableModel::setDocument(const ProjectDocument *document)
{
    beginResetModel();
    m_document = document;
    endResetModel();
}

int RegisterTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() || !m_document ? 0 : m_document->registers.size();
}

int RegisterTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 13;
}

QVariant RegisterTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_document || index.row() >= m_document->registers.size())
    {
        return QVariant();
    }
    const RegisterPoint &point = m_document->registers.at(index.row());
    if (role == Qt::ForegroundRole && !point.enabled) { return QBrush(QColor(QStringLiteral("#8a8983"))); }
    if (role != Qt::DisplayRole && role != Qt::EditRole) { return QVariant(); }
    switch (index.column())
    {
    case 0: return groupName(point.groupId);
    case 1: return point.slaveAddress;
    case 2: return point.address;
    case 3: return point.registerCount;
    case 4: return point.name;
    case 5: return dataTypeToString(point.dataType);
    case 6: return storageTypeToString(point.storageType);
    case 7: return point.currentValue.toDisplayString(point.precision);
    case 8: return point.protocolKey;
    case 9: return point.category;
    case 10: return point.label;
    case 11: return point.enabled ? QStringLiteral("是") : QStringLiteral("否");
    case 12: return strategyTypeToString(point.strategy.type);
    default: return QVariant();
    }
}

QVariant RegisterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) { return QVariant(); }
    static const QStringList headers = {QStringLiteral("分组"), QStringLiteral("从站"),
        QStringLiteral("地址"), QStringLiteral("数量"), QStringLiteral("名称"),
        QStringLiteral("类型"), QStringLiteral("存储区"), QStringLiteral("当前值"),
        QStringLiteral("协议键"), QStringLiteral("分类"), QStringLiteral("标签"),
        QStringLiteral("启用"), QStringLiteral("策略")};
    return headers.value(section);
}

QString RegisterTableModel::pointId(int row) const
{
    return m_document && row >= 0 && row < m_document->registers.size()
        ? m_document->registers.at(row).id : QString();
}

void RegisterTableModel::refreshPoint(const QString &pointId)
{
    if (!m_document) { return; }
    for (int row = 0; row < m_document->registers.size(); ++row)
    {
        if (m_document->registers.at(row).id == pointId)
        {
            emit dataChanged(index(row, 7), index(row, 7), {Qt::DisplayRole});
            return;
        }
    }
}

QString RegisterTableModel::groupName(const QString &groupId) const
{
    if (!m_document) { return QString(); }
    for (const RegisterGroup &group : m_document->groups)
    {
        if (group.id == groupId) { return group.name; }
    }
    return QString();
}
