#ifndef MODBUS_REGISTER_STORE_H
#define MODBUS_REGISTER_STORE_H

#include "Domain/Models/register_point.h"

#include <QHash>
#include <QList>
#include <QModbusDataUnit>
#include <QModbusPdu>
#include <QModbusResponse>
#include <QVector>

class ModbusRegisterStore
{
public:
    struct Block
    {
        int start = 0;
        QVector<quint16> values;

        int end() const { return start + values.size() - 1; }
    };

    struct SlaveTables
    {
        QVector<Block> holding;
        QVector<Block> input;
    };

    void clear();
    void build(const QList<RegisterPoint> &points, StorageType storageType);

    bool isEmpty() const;
    QList<quint8> slaveAddresses() const;
    int blockCount(QModbusDataUnit::RegisterType table) const;
    QString summary(QModbusDataUnit::RegisterType table) const;

    bool read(quint8 slaveAddress,
              QModbusDataUnit::RegisterType table,
              int address,
              int count,
              QVector<quint16> *out) const;
    bool write(quint8 slaveAddress,
               QModbusDataUnit::RegisterType table,
               int address,
               const QVector<quint16> &values);
    bool writeOne(quint8 slaveAddress,
                  QModbusDataUnit::RegisterType table,
                  int address,
                  quint16 value);
    bool readOne(quint8 slaveAddress,
                 QModbusDataUnit::RegisterType table,
                 int address,
                 quint16 *value) const;

    // 未知从站返回无效响应（由传输层决定不应答）
    QModbusResponse processRequest(quint8 slaveAddress, const QModbusPdu &request);

private:
    QVector<Block> *tableBlocks(quint8 slaveAddress, QModbusDataUnit::RegisterType table);
    const QVector<Block> *tableBlocks(quint8 slaveAddress, QModbusDataUnit::RegisterType table) const;
    Block *findBlock(QVector<Block> &blocks, int address);
    const Block *findBlock(const QVector<Block> &blocks, int address) const;
    static QVector<Block> mergeRanges(QVector<QPair<int, int>> ranges);
    static quint8 normalizedSlave(quint8 slaveAddress);

    QHash<quint8, SlaveTables> m_slaves;
};

#endif
