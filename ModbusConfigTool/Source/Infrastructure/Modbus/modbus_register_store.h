#ifndef MODBUS_REGISTER_STORE_H
#define MODBUS_REGISTER_STORE_H

#include "Domain/Models/register_point.h"

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

    void clear();
    void build(const QList<RegisterPoint> &points, StorageType storageType);

    bool isEmpty() const;
    int blockCount(QModbusDataUnit::RegisterType table) const;
    QString summary(QModbusDataUnit::RegisterType table) const;

    bool read(QModbusDataUnit::RegisterType table,
              int address,
              int count,
              QVector<quint16> *out) const;
    bool write(QModbusDataUnit::RegisterType table,
               int address,
               const QVector<quint16> &values);
    bool writeOne(QModbusDataUnit::RegisterType table, int address, quint16 value);
    bool readOne(QModbusDataUnit::RegisterType table, int address, quint16 *value) const;

    QModbusResponse processRequest(const QModbusPdu &request);

private:
    QVector<Block> *tableBlocks(QModbusDataUnit::RegisterType table);
    const QVector<Block> *tableBlocks(QModbusDataUnit::RegisterType table) const;
    Block *findBlock(QVector<Block> &blocks, int address);
    const Block *findBlock(const QVector<Block> &blocks, int address) const;
    static QVector<Block> mergeRanges(QVector<QPair<int, int>> ranges);

    QVector<Block> m_holding;
    QVector<Block> m_input;
};

#endif
