#include "modbus_register_store.h"

#include "Domain/Values/value_converter.h"

#include <QModbusExceptionResponse>
#include <QtAlgorithms>

#include <algorithm>

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

quint8 ModbusRegisterStore::normalizedSlave(quint8 slaveAddress)
{
    if (slaveAddress < 1 || slaveAddress > 247)
    {
        return 1;
    }
    return slaveAddress;
}

void ModbusRegisterStore::clear()
{
    m_slaves.clear();
}

QVector<ModbusRegisterStore::Block> ModbusRegisterStore::mergeRanges(QVector<QPair<int, int>> ranges)
{
    QVector<Block> blocks;
    if (ranges.isEmpty())
    {
        return blocks;
    }

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
    QHash<quint8, QVector<QPair<int, int>>> rangesBySlave;
    QHash<quint8, QList<RegisterPoint>> pointsBySlave;

    for (const RegisterPoint &point : points)
    {
        if (point.storageType != storageType)
        {
            continue;
        }
        const quint8 slave = normalizedSlave(point.slaveAddress);
        const int count = qMax(1, int(point.registerCount));
        rangesBySlave[slave].append(qMakePair(int(point.address), int(point.address) + count - 1));
        pointsBySlave[slave].append(point);
    }

    for (auto it = rangesBySlave.constBegin(); it != rangesBySlave.constEnd(); ++it)
    {
        const quint8 slave = it.key();
        SlaveTables &tables = m_slaves[slave];
        QVector<Block> blocks = mergeRanges(it.value());
        if (storageType == StorageType::Holding)
        {
            tables.holding = blocks;
        }
        else
        {
            tables.input = blocks;
        }
    }

    for (auto it = pointsBySlave.constBegin(); it != pointsBySlave.constEnd(); ++it)
    {
        const quint8 slave = it.key();
        for (const RegisterPoint &point : it.value())
        {
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
                writeOne(slave, table, int(point.address) + index, encoded.registers.at(index));
            }
        }
    }
}

QList<quint8> ModbusRegisterStore::slaveAddresses() const
{
    QList<quint8> addresses = m_slaves.keys();
    std::sort(addresses.begin(), addresses.end());
    return addresses;
}

bool ModbusRegisterStore::isEmpty() const
{
    for (auto it = m_slaves.constBegin(); it != m_slaves.constEnd(); ++it)
    {
        if (!it.value().holding.isEmpty() || !it.value().input.isEmpty())
        {
            return false;
        }
    }
    return true;
}

QVector<ModbusRegisterStore::Block> *ModbusRegisterStore::tableBlocks(
    quint8 slaveAddress, QModbusDataUnit::RegisterType table)
{
    const quint8 slave = normalizedSlave(slaveAddress);
    auto it = m_slaves.find(slave);
    if (it == m_slaves.end())
    {
        return nullptr;
    }
    if (table == QModbusDataUnit::HoldingRegisters)
    {
        return &it.value().holding;
    }
    if (table == QModbusDataUnit::InputRegisters)
    {
        return &it.value().input;
    }
    return nullptr;
}

const QVector<ModbusRegisterStore::Block> *ModbusRegisterStore::tableBlocks(
    quint8 slaveAddress, QModbusDataUnit::RegisterType table) const
{
    const quint8 slave = normalizedSlave(slaveAddress);
    auto it = m_slaves.constFind(slave);
    if (it == m_slaves.constEnd())
    {
        return nullptr;
    }
    if (table == QModbusDataUnit::HoldingRegisters)
    {
        return &it.value().holding;
    }
    if (table == QModbusDataUnit::InputRegisters)
    {
        return &it.value().input;
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

int ModbusRegisterStore::blockCount(QModbusDataUnit::RegisterType table) const
{
    int count = 0;
    for (auto it = m_slaves.constBegin(); it != m_slaves.constEnd(); ++it)
    {
        if (table == QModbusDataUnit::HoldingRegisters)
        {
            count += it.value().holding.size();
        }
        else if (table == QModbusDataUnit::InputRegisters)
        {
            count += it.value().input.size();
        }
    }
    return count;
}

QString ModbusRegisterStore::summary(QModbusDataUnit::RegisterType table) const
{
    QStringList parts;
    constexpr int limit = 6;
    int emitted = 0;
    for (quint8 slave : slaveAddresses())
    {
        const QVector<Block> *blocks = tableBlocks(slave, table);
        if (!blocks)
        {
            continue;
        }
        for (const Block &block : *blocks)
        {
            if (emitted >= limit)
            {
                break;
            }
            parts.append(QStringLiteral("S%1[%2-%3]")
                             .arg(slave)
                             .arg(block.start)
                             .arg(block.end()));
            ++emitted;
        }
        if (emitted >= limit)
        {
            break;
        }
    }
    const int total = blockCount(table);
    if (total > limit)
    {
        parts.append(QStringLiteral("...共%1块").arg(total));
    }
    return parts.join(QStringLiteral(", "));
}

bool ModbusRegisterStore::readOne(quint8 slaveAddress,
                                  QModbusDataUnit::RegisterType table,
                                  int address,
                                  quint16 *value) const
{
    const QVector<Block> *blocks = tableBlocks(slaveAddress, table);
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

bool ModbusRegisterStore::writeOne(quint8 slaveAddress,
                                   QModbusDataUnit::RegisterType table,
                                   int address,
                                   quint16 value)
{
    QVector<Block> *blocks = tableBlocks(slaveAddress, table);
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

bool ModbusRegisterStore::read(quint8 slaveAddress,
                               QModbusDataUnit::RegisterType table,
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
        if (!readOne(slaveAddress, table, address + index, &(*out)[index]))
        {
            out->clear();
            return false;
        }
    }
    return true;
}

bool ModbusRegisterStore::write(quint8 slaveAddress,
                                QModbusDataUnit::RegisterType table,
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
        if (!readOne(slaveAddress, table, address + index, &ignored))
        {
            return false;
        }
    }
    for (int index = 0; index < values.size(); ++index)
    {
        if (!writeOne(slaveAddress, table, address + index, values.at(index)))
        {
            return false;
        }
    }
    return true;
}

QModbusResponse ModbusRegisterStore::processRequest(quint8 slaveAddress, const QModbusPdu &request)
{
    const quint8 slave = normalizedSlave(slaveAddress);
    if (!m_slaves.contains(slave))
    {
        // 该从站未映射：返回无效响应，由传输层保持静默（RTU 标准行为）
        return QModbusResponse();
    }

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
        if (!read(slave, table, int(address), int(count), &values))
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
        if (!writeOne(slave, QModbusDataUnit::HoldingRegisters, int(address), value))
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
        if (!write(slave, QModbusDataUnit::HoldingRegisters, int(address), values))
        {
            return illegalAddress(functionCode);
        }
        return QModbusResponse(functionCode, address, count);
    }
    default:
        return illegalFunction(functionCode);
    }
}
