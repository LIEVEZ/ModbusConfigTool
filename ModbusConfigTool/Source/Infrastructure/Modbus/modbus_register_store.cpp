#include "modbus_register_store.h"

#include "Domain/Values/value_converter.h"

#include <QModbusExceptionResponse>
#include <QtAlgorithms>

namespace
{
QModbusExceptionResponse illegalAddress(QModbusPdu::FunctionCode functionCode)
{
    return QModbusExceptionResponse(functionCode, QModbusPdu::IllegalDataAddress);
}

QModbusExceptionResponse illegalFunction(QModbusPdu::FunctionCode functionCode)
{
    return QModbusExceptionResponse(functionCode, QModbusPdu::IllegalFunction);
}

QModbusExceptionResponse illegalValue(QModbusPdu::FunctionCode functionCode)
{
    return QModbusExceptionResponse(functionCode, QModbusPdu::IllegalDataValue);
}
}

void ModbusRegisterStore::clear()
{
    m_holding.clear();
    m_input.clear();
}

QVector<ModbusRegisterStore::Block> ModbusRegisterStore::mergeRanges(QVector<QPair<int, int>> ranges)
{
    QVector<Block> blocks;
    if (ranges.isEmpty())
    {
        return blocks;
    }

    // Small holes (<= kMaxGapFill addresses) are absorbed into one block and stay 0.
    // Large gaps remain separate sparse blocks to avoid huge allocations.
    constexpr int kMaxGapFill = 16;

    std::sort(ranges.begin(), ranges.end(), [](const QPair<int, int> &left, const QPair<int, int> &right) {
        return left.first < right.first;
    });

    int currentStart = ranges.first().first;
    int currentEnd = ranges.first().second;
    for (int index = 1; index < ranges.size(); ++index)
    {
        const int start = ranges.at(index).first;
        const int end = ranges.at(index).second;
        // holeCount = start - currentEnd - 1; merge when holeCount <= kMaxGapFill
        // (also covers overlap / adjacent: holeCount <= 0).
        if (start <= currentEnd + 1 + kMaxGapFill)
        {
            currentEnd = qMax(currentEnd, end);
            continue;
        }

        Block block;
        block.start = currentStart;
        block.values = QVector<quint16>(qMax(0, currentEnd - currentStart + 1), 0);
        blocks.append(block);
        currentStart = start;
        currentEnd = end;
    }

    Block block;
    block.start = currentStart;
    block.values = QVector<quint16>(qMax(0, currentEnd - currentStart + 1), 0);
    blocks.append(block);
    return blocks;
}

void ModbusRegisterStore::build(const QList<RegisterPoint> &points, StorageType storageType)
{
    QVector<QPair<int, int>> ranges;
    for (const RegisterPoint &point : points)
    {
        if (point.storageType != storageType)
        {
            continue;
        }
        const int count = qMax(1, int(point.registerCount));
        ranges.append(qMakePair(int(point.address), int(point.address) + count - 1));
    }

    QVector<Block> blocks = mergeRanges(ranges);
    if (storageType == StorageType::Holding)
    {
        m_holding = blocks;
    }
    else
    {
        m_input = blocks;
    }

    for (const RegisterPoint &point : points)
    {
        if (point.storageType != storageType)
        {
            continue;
        }

        RegisterValue value = point.currentValue;
        if (value.dataType() != point.dataType)
        {
            value = RegisterValue::fromUnsigned64(value.toUnsigned64(), point.dataType);
        }
        const ConversionResult encoded = ValueConverter::toRegisters(value, point.endian);
        const QModbusDataUnit::RegisterType table = storageType == StorageType::Holding
            ? QModbusDataUnit::HoldingRegisters
            : QModbusDataUnit::InputRegisters;
        for (int index = 0; index < encoded.registers.size(); ++index)
        {
            writeOne(table, int(point.address) + index, encoded.registers.at(index));
        }
    }
}

QVector<ModbusRegisterStore::Block> *ModbusRegisterStore::tableBlocks(QModbusDataUnit::RegisterType table)
{
    if (table == QModbusDataUnit::HoldingRegisters)
    {
        return &m_holding;
    }
    if (table == QModbusDataUnit::InputRegisters)
    {
        return &m_input;
    }
    return nullptr;
}

const QVector<ModbusRegisterStore::Block> *ModbusRegisterStore::tableBlocks(
    QModbusDataUnit::RegisterType table) const
{
    if (table == QModbusDataUnit::HoldingRegisters)
    {
        return &m_holding;
    }
    if (table == QModbusDataUnit::InputRegisters)
    {
        return &m_input;
    }
    return nullptr;
}

ModbusRegisterStore::Block *ModbusRegisterStore::findBlock(QVector<Block> &blocks, int address)
{
    for (Block &block : blocks)
    {
        if (address >= block.start && address <= block.end())
        {
            return &block;
        }
    }
    return nullptr;
}

const ModbusRegisterStore::Block *ModbusRegisterStore::findBlock(const QVector<Block> &blocks,
                                                                int address) const
{
    for (const Block &block : blocks)
    {
        if (address >= block.start && address <= block.end())
        {
            return &block;
        }
    }
    return nullptr;
}

bool ModbusRegisterStore::isEmpty() const
{
    return m_holding.isEmpty() && m_input.isEmpty();
}

int ModbusRegisterStore::blockCount(QModbusDataUnit::RegisterType table) const
{
    const QVector<Block> *blocks = tableBlocks(table);
    return blocks ? blocks->size() : 0;
}

QString ModbusRegisterStore::summary(QModbusDataUnit::RegisterType table) const
{
    const QVector<Block> *blocks = tableBlocks(table);
    if (!blocks || blocks->isEmpty())
    {
        return QStringLiteral("无");
    }

    QStringList parts;
    const int limit = qMin(8, blocks->size());
    for (int index = 0; index < limit; ++index)
    {
        const Block &block = blocks->at(index);
        parts.append(QStringLiteral("[%1-%2]").arg(block.start).arg(block.end()));
    }
    if (blocks->size() > limit)
    {
        parts.append(QStringLiteral("...共%1块").arg(blocks->size()));
    }
    return parts.join(QStringLiteral(", "));
}

bool ModbusRegisterStore::readOne(QModbusDataUnit::RegisterType table, int address, quint16 *value) const
{
    const QVector<Block> *blocks = tableBlocks(table);
    if (!blocks || !value)
    {
        return false;
    }
    const Block *block = findBlock(*blocks, address);
    if (!block)
    {
        return false;
    }
    *value = block->values.at(address - block->start);
    return true;
}

bool ModbusRegisterStore::writeOne(QModbusDataUnit::RegisterType table, int address, quint16 value)
{
    QVector<Block> *blocks = tableBlocks(table);
    if (!blocks)
    {
        return false;
    }
    Block *block = findBlock(*blocks, address);
    if (!block)
    {
        return false;
    }
    block->values[address - block->start] = value;
    return true;
}

bool ModbusRegisterStore::read(QModbusDataUnit::RegisterType table,
                               int address,
                               int count,
                               QVector<quint16> *out) const
{
    if (!out || count <= 0 || count > 125)
    {
        return false;
    }
    out->resize(count);
    for (int index = 0; index < count; ++index)
    {
        if (!readOne(table, address + index, &(*out)[index]))
        {
            out->clear();
            return false;
        }
    }
    return true;
}

bool ModbusRegisterStore::write(QModbusDataUnit::RegisterType table,
                                int address,
                                const QVector<quint16> &values)
{
    if (values.isEmpty() || values.size() > 123)
    {
        return false;
    }
    for (int index = 0; index < values.size(); ++index)
    {
        quint16 ignored = 0;
        if (!readOne(table, address + index, &ignored))
        {
            return false;
        }
    }
    for (int index = 0; index < values.size(); ++index)
    {
        if (!writeOne(table, address + index, values.at(index)))
        {
            return false;
        }
    }
    return true;
}

QModbusResponse ModbusRegisterStore::processRequest(const QModbusPdu &request)
{
    const QModbusPdu::FunctionCode functionCode = request.functionCode();
    switch (functionCode)
    {
    case QModbusPdu::ReadHoldingRegisters:
    case QModbusPdu::ReadInputRegisters:
    {
        if (request.dataSize() != 4)
        {
            return illegalValue(functionCode);
        }
        quint16 address = 0;
        quint16 count = 0;
        request.decodeData(&address, &count);
        if (count == 0 || count > 125)
        {
            return illegalValue(functionCode);
        }
        const QModbusDataUnit::RegisterType table = functionCode == QModbusPdu::ReadHoldingRegisters
            ? QModbusDataUnit::HoldingRegisters
            : QModbusDataUnit::InputRegisters;
        QVector<quint16> values;
        if (!read(table, int(address), int(count), &values))
        {
            return illegalAddress(functionCode);
        }
        return QModbusResponse(functionCode, quint8(values.size() * 2), values);
    }
    case QModbusPdu::WriteSingleRegister:
    {
        if (request.dataSize() != 4)
        {
            return illegalValue(functionCode);
        }
        quint16 address = 0;
        quint16 value = 0;
        request.decodeData(&address, &value);
        if (!writeOne(QModbusDataUnit::HoldingRegisters, int(address), value))
        {
            return illegalAddress(functionCode);
        }
        return QModbusResponse(functionCode, address, value);
    }
    case QModbusPdu::WriteMultipleRegisters:
    {
        if (request.dataSize() < 5)
        {
            return illegalValue(functionCode);
        }
        quint16 address = 0;
        quint16 count = 0;
        quint8 byteCount = 0;
        request.decodeData(&address, &count, &byteCount);
        if (count == 0 || count > 123 || byteCount != quint8(count * 2) || request.dataSize() != 5 + byteCount)
        {
            return illegalValue(functionCode);
        }
        const QByteArray payload = request.data().mid(5);
        if (payload.size() != byteCount)
        {
            return illegalValue(functionCode);
        }
        QVector<quint16> values(count);
        for (int index = 0; index < count; ++index)
        {
            values[index] = quint16((quint8(payload.at(index * 2)) << 8) | quint8(payload.at(index * 2 + 1)));
        }
        if (!write(QModbusDataUnit::HoldingRegisters, int(address), values))
        {
            return illegalAddress(functionCode);
        }
        return QModbusResponse(functionCode, address, count);
    }
    default:
        return illegalFunction(functionCode);
    }
}
