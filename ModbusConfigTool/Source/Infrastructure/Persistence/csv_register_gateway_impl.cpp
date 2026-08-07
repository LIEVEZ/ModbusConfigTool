#include "csv_register_gateway_impl.h"

#include "Domain/Models/project_factory.h"
#include "Domain/Validation/validation_service.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <QUuid>

namespace
{
QString quoteCsv(const QString &text)
{
    QString escaped = text;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

// quotesBalanced 为可空输出参数：解析结束时若仍处于引号内部则置 false，
// 用于判断该文本是否为一条完整记录（引号内允许换行，记录可能跨多个物理行）。
QStringList parseCsvLine(const QString &line, bool *quotesBalanced = nullptr)
{
    QStringList fields;
    QString field;
    bool quoted = false;
    for (int index = 0; index < line.size(); ++index)
    {
        const QChar character = line.at(index);
        if (character == QLatin1Char('"'))
        {
            if (quoted && index + 1 < line.size() && line.at(index + 1) == QLatin1Char('"'))
            {
                field += character;
                ++index;
            }
            else
            {
                quoted = !quoted;
            }
        }
        else if (character == QLatin1Char(',') && !quoted)
        {
            fields.append(field);
            field.clear();
        }
        else
        {
            field += character;
        }
    }
    fields.append(field);
    if (quotesBalanced)
    {
        *quotesBalanced = !quoted;
    }
    return fields;
}

QStringList normalizeHeaders(const QStringList &rawHeaders)
{
    QStringList headers;
    headers.reserve(rawHeaders.size());
    for (const QString &header : rawHeaders)
    {
        headers.append(header.trimmed());
    }
    return headers;
}

int headerIndex(const QStringList &headers, const QStringList &aliases)
{
    for (const QString &alias : aliases)
    {
        const int index = headers.indexOf(alias);
        if (index >= 0)
        {
            return index;
        }
    }
    return -1;
}

bool hasAnyHeader(const QStringList &headers, const QStringList &aliases)
{
    return headerIndex(headers, aliases) >= 0;
}

QString fieldByAliases(const QStringList &headers,
                       const QStringList &fields,
                       const QStringList &aliases)
{
    const int index = headerIndex(headers, aliases);
    return index >= 0 ? fields.value(index).trimmed() : QString();
}

bool parseUIntFlexible(const QString &text, uint *value)
{
    if (!value)
    {
        return false;
    }
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
    {
        return false;
    }

    bool ok = false;
    if (trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
    {
        *value = trimmed.mid(2).toUInt(&ok, 16);
        return ok;
    }

    *value = trimmed.toUInt(&ok, 10);
    return ok;
}

bool parseIntFlexible(const QString &text, int *value)
{
    if (!value)
    {
        return false;
    }
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
    {
        return false;
    }

    bool ok = false;
    if (trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
    {
        *value = trimmed.mid(2).toInt(&ok, 16);
        return ok;
    }

    *value = trimmed.toInt(&ok, 10);
    return ok;
}

bool parseBoolFlexible(const QString &text, bool defaultValue)
{
    const QString value = text.trimmed().toLower();
    if (value.isEmpty())
    {
        return defaultValue;
    }
    if (value == QStringLiteral("1")
        || value == QStringLiteral("true")
        || value == QStringLiteral("yes")
        || value == QStringLiteral("y")
        || value == QStringLiteral("on"))
    {
        return true;
    }
    if (value == QStringLiteral("0")
        || value == QStringLiteral("false")
        || value == QStringLiteral("no")
        || value == QStringLiteral("n")
        || value == QStringLiteral("off"))
    {
        return false;
    }
    return defaultValue;
}

bool rangesOverlap(quint8 slave,
                   StorageType storage,
                   quint16 address,
                   quint16 count,
                   const RegisterPoint &other)
{
    if (other.slaveAddress != slave || other.storageType != storage)
    {
        return false;
    }
    const quint32 end = quint32(address) + count - 1U;
    const quint32 otherEnd = quint32(other.address) + other.registerCount - 1U;
    return address <= otherEnd && other.address <= end;
}
} // namespace

CsvImportResult CsvRegisterGatewayImpl::importFile(const QString &path,
                                                    const ProjectDocument &current) const
{
    CsvImportResult output;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        output.result = OperationResult::fail(
            QStringLiteral("csv_open_failed"),
            QStringLiteral("path"),
            QStringLiteral("无法打开 CSV 文件"),
            file.errorString());
        return output;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QString headerLine = stream.readLine();
    if (!headerLine.isEmpty() && headerLine.at(0) == QChar(0xFEFF))
    {
        headerLine.remove(0, 1);
    }

    const QStringList headers = normalizeHeaders(parseCsvLine(headerLine));

    const QStringList addressAliases = {
        QStringLiteral("address"), QStringLiteral("start_addr"),
        QStringLiteral("start_address"), QStringLiteral("reg_addr"),
        QStringLiteral("register_address")
    };
    const QStringList nameAliases = {
        QStringLiteral("name"), QStringLiteral("point_name")
    };
    const QStringList dataTypeAliases = {
        QStringLiteral("data_type"), QStringLiteral("datatype"), QStringLiteral("type")
    };
    const QStringList slaveAliases = {
        QStringLiteral("slave_address"), QStringLiteral("slave_addr"),
        QStringLiteral("slave"), QStringLiteral("unit_id")
    };
    const QStringList protocolAliases = {
        QStringLiteral("protocol_key"), QStringLiteral("key"), QStringLiteral("point_key")
    };
    const QStringList groupAliases = {
        QStringLiteral("group_name"), QStringLiteral("group"), QStringLiteral("group_id")
    };
    const QStringList endianAliases = {
        QStringLiteral("endian"), QStringLiteral("encode_mode"), QStringLiteral("byte_order")
    };
    const QStringList quantityAliases = {
        QStringLiteral("register_count"), QStringLiteral("quantity"), QStringLiteral("count")
    };
    const QStringList readFcAliases = {
        QStringLiteral("read_function_code"), QStringLiteral("read"),
        QStringLiteral("fc_read"), QStringLiteral("function_code")
    };
    const QStringList writeFcAliases = {
        QStringLiteral("write_function_code"), QStringLiteral("write"), QStringLiteral("fc_write")
    };
    const QStringList currentValueAliases = {
        QStringLiteral("current_value"), QStringLiteral("value"), QStringLiteral("default_value")
    };
    const QStringList enabledAliases = {
        QStringLiteral("enabled"), QStringLiteral("upload")
    };

    if (!hasAnyHeader(headers, addressAliases))
    {
        output.result = OperationResult::fail(
            QStringLiteral("missing_column"),
            QStringLiteral("address"),
            QStringLiteral("CSV 缺少必填列：address / start_addr"));
        return output;
    }
    if (!hasAnyHeader(headers, nameAliases))
    {
        output.result = OperationResult::fail(
            QStringLiteral("missing_column"),
            QStringLiteral("name"),
            QStringLiteral("CSV 缺少必填列：name"));
        return output;
    }
    if (!hasAnyHeader(headers, dataTypeAliases))
    {
        output.result = OperationResult::fail(
            QStringLiteral("missing_column"),
            QStringLiteral("data_type"),
            QStringLiteral("CSV 缺少必填列：data_type"));
        return output;
    }

    output.groups = current.groups;

    int lineNumber = 1;
    int skippedRows = 0;
    QStringList skipNotes;
    QString recordText;
    int recordStartLine = 0;

    // 处理一条完整记录（引号内允许换行，一条记录可能跨多个物理行）。
    // 返回 false 表示已设置致命错误，导入应立即中止。
    const auto handleRecord = [&](const QStringList &fields, int rowLine) -> bool {
        if (fields.size() != headers.size())
        {
            output.result = OperationResult::fail(
                QStringLiteral("column_count"),
                QStringLiteral("line"),
                QStringLiteral("CSV 第 %1 行列数不匹配").arg(rowLine));
            return false;
        }

        const auto field = [&](const QStringList &aliases) {
            return fieldByAliases(headers, fields, aliases);
        };

        const QString dataTypeText = field(dataTypeAliases);
        if (dataTypeText.isEmpty())
        {
            ++skippedRows;
            if (skipNotes.size() < 8)
            {
                skipNotes.append(QStringLiteral("第%1行：缺少数据类型（块定义已跳过）").arg(rowLine));
            }
            return true;
        }

        DataType dataType = DataType::UInt16;
        if (!dataTypeFromString(dataTypeText, &dataType))
        {
            output.result = OperationResult::fail(
                QStringLiteral("invalid_type"),
                QStringLiteral("data_type"),
                QStringLiteral("CSV 第 %1 行数据类型无效：%2").arg(rowLine).arg(dataTypeText));
            return false;
        }

        const quint16 expectedCount = ProjectFactory::registerCountFor(dataType);
        if (hasAnyHeader(headers, quantityAliases))
        {
            uint quantity = 0;
            if (parseUIntFlexible(field(quantityAliases), &quantity)
                && quantity > 0
                && quantity != expectedCount)
            {
                ++skippedRows;
                if (skipNotes.size() < 8)
                {
                    skipNotes.append(
                        QStringLiteral("第%1行：quantity=%2 与类型宽度%3不符，已跳过")
                            .arg(rowLine)
                            .arg(quantity)
                            .arg(expectedCount));
                }
                return true;
            }
        }

        QString groupId;
        const QString groupName = field(groupAliases);
        if (!groupName.isEmpty())
        {
            for (const RegisterGroup &group : output.groups)
            {
                if (group.name == groupName)
                {
                    groupId = group.id;
                    break;
                }
            }
            if (groupId.isEmpty())
            {
                RegisterGroup group;
                group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                group.name = groupName;
                output.groups.append(group);
                groupId = group.id;
            }
        }
        else if (!output.groups.isEmpty())
        {
            groupId = output.groups.first().id;
        }
        else
        {
            RegisterGroup group;
            group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            group.name = QStringLiteral("导入分组");
            output.groups.append(group);
            groupId = group.id;
        }

        uint addressValue = 0;
        if (!parseUIntFlexible(field(addressAliases), &addressValue))
        {
            output.result = OperationResult::fail(
                QStringLiteral("invalid_address"),
                QStringLiteral("address"),
                QStringLiteral("CSV 第 %1 行地址无效").arg(rowLine));
            return false;
        }

        RegisterPoint point = ProjectFactory::createRegister(groupId, quint16(addressValue));
        point.dataType = dataType;
        point.registerCount = expectedCount;
        point.minimumValue = ProjectFactory::minimumFor(point.dataType);
        point.maximumValue = ProjectFactory::maximumFor(point.dataType);
        point.name.clear();
        point.protocolKey.clear();
        point.writeFunctionCode = 0;

        // 从站地址非法或越界时不中断导入，统一回落为 1（有效范围 1～247）
        uint slaveValue = 1;
        if (hasAnyHeader(headers, slaveAliases))
        {
            const QString slaveText = field(slaveAliases).trimmed();
            if (!slaveText.isEmpty())
            {
                uint parsedSlave = 1;
                if (parseUIntFlexible(slaveText, &parsedSlave)
                    && parsedSlave >= 1
                    && parsedSlave <= 247)
                {
                    slaveValue = parsedSlave;
                }
                else
                {
                    slaveValue = 1;
                }
            }
        }
        point.slaveAddress = quint8(slaveValue);

        // CSV 空字段保持为空；有值则原样保留，不自动生成、不加后缀。
        point.name = field(nameAliases).trimmed();
        point.protocolKey = field(protocolAliases).trimmed();

        if (hasAnyHeader(headers, endianAliases))
        {
            endianFromString(field(endianAliases), &point.endian);
        }
        if (headers.contains(QStringLiteral("storage_type")))
        {
            storageTypeFromString(field({QStringLiteral("storage_type")}), &point.storageType);
        }
        if (hasAnyHeader(headers, readFcAliases))
        {
            uint readCode = 0;
            if (parseUIntFlexible(field(readFcAliases), &readCode))
            {
                point.readFunctionCode = quint8(readCode);
                if (readCode == 4)
                {
                    point.storageType = StorageType::Input;
                }
                else if (readCode == 3)
                {
                    point.storageType = StorageType::Holding;
                }
            }
        }
        // 写码未填时保持为空（0），不默认填 0x06。
        point.writeFunctionCode = 0;
        if (hasAnyHeader(headers, writeFcAliases))
        {
            const QString writeText = field(writeFcAliases).trimmed();
            int writeCode = 0;
            if (!writeText.isEmpty() && parseIntFlexible(writeText, &writeCode))
            {
                point.writeFunctionCode = writeCode;
            }
        }
        if (headers.contains(QStringLiteral("unit")))
        {
            point.unit = field({QStringLiteral("unit")});
        }
        if (headers.contains(QStringLiteral("offset")))
        {
            point.offset = field({QStringLiteral("offset")}).toDouble();
        }
        if (headers.contains(QStringLiteral("precision")))
        {
            point.precision = field({QStringLiteral("precision")}).toInt();
        }
        if (headers.contains(QStringLiteral("min_value")))
        {
            const ValueResult minimum =
                RegisterValue::fromText(point.dataType, field({QStringLiteral("min_value")}));
            if (!minimum.result.success)
            {
                output.result = minimum.result;
                return false;
            }
            point.minimumValue = minimum.value;
        }
        if (headers.contains(QStringLiteral("max_value")))
        {
            const ValueResult maximum =
                RegisterValue::fromText(point.dataType, field({QStringLiteral("max_value")}));
            if (!maximum.result.success)
            {
                output.result = maximum.result;
                return false;
            }
            point.maximumValue = maximum.value;
        }
        if (hasAnyHeader(headers, currentValueAliases))
        {
            const QString currentText = field(currentValueAliases);
            if (!currentText.isEmpty())
            {
                const ValueResult value = RegisterValue::fromText(point.dataType, currentText);
                if (!value.result.success)
                {
                    output.result = value.result;
                    return false;
                }
                point.currentValue = value.value;
            }
        }
        if (hasAnyHeader(headers, enabledAliases))
        {
            point.enabled = true; // 点位启用字段已废弃，导入后一律生效
        }
        if (headers.contains(QStringLiteral("category")))
        {
            point.category = field({QStringLiteral("category")});
        }
        if (headers.contains(QStringLiteral("label")))
        {
            point.label = field({QStringLiteral("label")});
        }
        if (headers.contains(QStringLiteral("strategy_type")))
        {
            strategyTypeFromString(field({QStringLiteral("strategy_type")}), &point.strategy.type);
        }
        if (headers.contains(QStringLiteral("strategy_enabled")))
        {
            point.strategy.enabled =
                parseBoolFlexible(field({QStringLiteral("strategy_enabled")}), false);
        }
        if (headers.contains(QStringLiteral("strategy_params")))
        {
            const QJsonDocument parameters =
                QJsonDocument::fromJson(field({QStringLiteral("strategy_params")}).toUtf8());
            if (parameters.isObject())
            {
                point.strategy.parameters = parameters.object().toVariantMap();
            }
        }

        // 仅检查本文件内地址重叠；跨分组冲突交给工程校验
        // （同端口且同时启用才报错，未绑定端口不拦截）
        bool overlapped = false;
        for (const RegisterPoint &existing : output.registers)
        {
            if (rangesOverlap(point.slaveAddress, point.storageType, point.address,
                              point.registerCount, existing))
            {
                overlapped = true;
                break;
            }
        }
        if (overlapped)
        {
            ++skippedRows;
            if (skipNotes.size() < 8)
            {
                skipNotes.append(QStringLiteral("第%1行：地址 %2 与本文件内已有点重叠，已跳过")
                                     .arg(rowLine)
                                     .arg(point.address));
            }
            return true;
        }

        output.registers.append(point);
        return true;
    };

    while (!stream.atEnd() || !recordText.isEmpty())
    {
        if (!stream.atEnd())
        {
            ++lineNumber;
            const QString line = stream.readLine();
            if (recordText.isEmpty())
            {
                if (line.trimmed().isEmpty())
                {
                    continue; // 空行跳过；跨行单元格内部的空行由拼接逻辑保留
                }
                recordStartLine = lineNumber;
            }
            if (!recordText.isEmpty())
            {
                recordText += QLatin1Char('\n');
            }
            recordText += line;
        }

        bool quotesBalanced = true;
        const QStringList fields = parseCsvLine(recordText, &quotesBalanced);
        if (!quotesBalanced && !stream.atEnd())
        {
            continue; // 引号未闭合，说明单元格跨行，继续拼接下一物理行
        }
        recordText.clear();
        if (!handleRecord(fields, recordStartLine))
        {
            return output;
        }
    }

    if (output.registers.isEmpty())
    {
        output.result = OperationResult::fail(
            QStringLiteral("csv_empty"),
            QStringLiteral("registers"),
            QStringLiteral("CSV 未解析到有效寄存器点位"),
            skipNotes.join(QLatin1Char(';')));
        return output;
    }

    // 逐点基础校验；跨组/同端口冲突由 RegisterService 合入工程后再 validateProject
    for (const RegisterPoint &point : output.registers)
    {
        const OperationResult pointResult = ValidationService::validateRegister(point);
        if (!pointResult.success)
        {
            output.result = pointResult;
            return output;
        }
    }

    output.result = OperationResult::ok();
    if (skippedRows > 0)
    {
        output.result.message = QStringLiteral("已解析 %1 条，跳过 %2 条")
                                    .arg(output.registers.size())
                                    .arg(skippedRows);
        output.result.detail = skipNotes.join(QLatin1Char(';'));
    }
    Q_UNUSED(current);
    return output;
}

OperationResult CsvRegisterGatewayImpl::exportFile(const QString &path,
                                                   const ProjectDocument &document) const
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return OperationResult::fail(
            QStringLiteral("csv_save_failed"),
            QStringLiteral("path"),
            QStringLiteral("无法写入 CSV 文件"),
            file.errorString());
    }

    file.write("\xEF\xBB\xBF");
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << "group_name,slave_address,address,register_count,name,data_type,endian,storage_type,"
              "read_function_code,write_function_code,protocol_key,unit,offset,precision,"
              "min_value,max_value,current_value,enabled,category,label,strategy_type,"
              "strategy_enabled,strategy_params\n";

    for (const RegisterPoint &point : document.registers)
    {
        QString groupName;
        for (const RegisterGroup &group : document.groups)
        {
            if (group.id == point.groupId)
            {
                groupName = group.name;
                break;
            }
        }
        const QString strategyParameters = QString::fromUtf8(
            QJsonDocument(QJsonObject::fromVariantMap(point.strategy.parameters))
                .toJson(QJsonDocument::Compact));
        stream << quoteCsv(groupName) << ','
               << point.slaveAddress << ','
               << point.address << ','
               << point.registerCount << ','
               << quoteCsv(point.name) << ','
               << dataTypeToString(point.dataType) << ','
               << endianToString(point.endian) << ','
               << storageTypeToString(point.storageType) << ','
               << point.readFunctionCode << ','
               << point.writeFunctionCode << ','
               << quoteCsv(point.protocolKey) << ','
               << quoteCsv(point.unit) << ','
               << point.offset << ','
               << point.precision << ','
               << quoteCsv(point.minimumValue.toStorageString()) << ','
               << quoteCsv(point.maximumValue.toStorageString()) << ','
               << quoteCsv(point.currentValue.toStorageString()) << ','
               << (point.enabled ? "true" : "false") << ','
               << quoteCsv(point.category) << ','
               << quoteCsv(point.label) << ','
               << strategyTypeToString(point.strategy.type) << ','
               << (point.strategy.enabled ? "true" : "false") << ','
               << quoteCsv(strategyParameters) << '\n';
    }

    stream.flush();
    if (!file.commit())
    {
        return OperationResult::fail(
            QStringLiteral("csv_commit_failed"),
            QStringLiteral("path"),
            QStringLiteral("无法原子保存 CSV 文件"),
            file.errorString());
    }
    return OperationResult::ok();
}
