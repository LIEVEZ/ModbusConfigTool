#include "csv_register_gateway_impl.h"

#include "Domain/Models/project_factory.h"
#include "Domain/Validation/validation_service.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTextStream>
#include <QUuid>

namespace
{
QString quoteCsv(const QString &text)
{
    QString escaped = text; escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

QStringList parseCsvLine(const QString &line)
{
    QStringList fields; QString field; bool quoted = false;
    for (int index = 0; index < line.size(); ++index)
    {
        const QChar character = line.at(index);
        if (character == QLatin1Char('"'))
        {
            if (quoted && index + 1 < line.size() && line.at(index + 1) == QLatin1Char('"')) { field += character; ++index; }
            else { quoted = !quoted; }
        }
        else if (character == QLatin1Char(',') && !quoted) { fields.append(field); field.clear(); }
        else { field += character; }
    }
    fields.append(field); return fields;
}
}

CsvImportResult CsvRegisterGatewayImpl::importFile(const QString &path,
                                                    const ProjectDocument &current) const
{
    CsvImportResult output; QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        output.result = OperationResult::fail(QStringLiteral("csv_open_failed"), QStringLiteral("path"), QStringLiteral("无法打开 CSV 文件"), file.errorString()); return output;
    }
    QTextStream stream(&file); stream.setCodec("UTF-8");
    QString headerLine = stream.readLine(); if (!headerLine.isEmpty() && headerLine.at(0) == QChar(0xFEFF)) { headerLine.remove(0, 1); }
    const QStringList headers = parseCsvLine(headerLine);
    const QStringList required = {QStringLiteral("group_name"), QStringLiteral("slave_address"), QStringLiteral("address"), QStringLiteral("name"), QStringLiteral("data_type"), QStringLiteral("protocol_key"), QStringLiteral("current_value")};
    for (const QString &name : required)
    {
        if (!headers.contains(name)) { output.result = OperationResult::fail(QStringLiteral("missing_column"), name, QStringLiteral("CSV 缺少必填列：%1").arg(name)); return output; }
    }
    output.groups = current.groups; int lineNumber = 1;
    while (!stream.atEnd())
    {
        ++lineNumber; const QString line = stream.readLine(); if (line.trimmed().isEmpty()) { continue; }
        const QStringList fields = parseCsvLine(line);
        if (fields.size() != headers.size()) { output.result = OperationResult::fail(QStringLiteral("column_count"), QStringLiteral("line"), QStringLiteral("CSV 第 %1 行列数不匹配").arg(lineNumber)); return output; }
        const auto field = [&headers, &fields](const QString &name) { return fields.value(headers.indexOf(name)); };
        QString groupId; const QString groupName = field(QStringLiteral("group_name")).trimmed();
        for (const RegisterGroup &group : output.groups) { if (group.name == groupName) { groupId = group.id; break; } }
        if (groupId.isEmpty()) { RegisterGroup group; group.id = QUuid::createUuid().toString(QUuid::WithoutBraces); group.name = groupName; output.groups.append(group); groupId = group.id; }
        RegisterPoint point = ProjectFactory::createRegister(groupId, quint16(field(QStringLiteral("address")).toUInt()));
        point.slaveAddress = quint8(field(QStringLiteral("slave_address")).toUInt()); point.name = field(QStringLiteral("name")); point.protocolKey = field(QStringLiteral("protocol_key"));
        if (!dataTypeFromString(field(QStringLiteral("data_type")), &point.dataType)) { output.result = OperationResult::fail(QStringLiteral("invalid_type"), QStringLiteral("data_type"), QStringLiteral("CSV 第 %1 行数据类型无效").arg(lineNumber)); return output; }
        point.registerCount = ProjectFactory::registerCountFor(point.dataType); point.minimumValue = ProjectFactory::minimumFor(point.dataType); point.maximumValue = ProjectFactory::maximumFor(point.dataType);
        if (headers.contains(QStringLiteral("endian"))) { endianFromString(field(QStringLiteral("endian")), &point.endian); }
        if (headers.contains(QStringLiteral("storage_type"))) { storageTypeFromString(field(QStringLiteral("storage_type")), &point.storageType); }
        if (headers.contains(QStringLiteral("read_function_code"))) { point.readFunctionCode = quint8(field(QStringLiteral("read_function_code")).toUInt()); }
        if (headers.contains(QStringLiteral("write_function_code"))) { point.writeFunctionCode = field(QStringLiteral("write_function_code")).toInt(); }
        if (headers.contains(QStringLiteral("unit"))) { point.unit = field(QStringLiteral("unit")); }
        if (headers.contains(QStringLiteral("offset"))) { point.offset = field(QStringLiteral("offset")).toDouble(); }
        if (headers.contains(QStringLiteral("precision"))) { point.precision = field(QStringLiteral("precision")).toInt(); }
        if (headers.contains(QStringLiteral("min_value")))
        {
            const ValueResult minimum = RegisterValue::fromText(point.dataType, field(QStringLiteral("min_value")));
            if (!minimum.result.success) { output.result = minimum.result; return output; }
            point.minimumValue = minimum.value;
        }
        if (headers.contains(QStringLiteral("max_value")))
        {
            const ValueResult maximum = RegisterValue::fromText(point.dataType, field(QStringLiteral("max_value")));
            if (!maximum.result.success) { output.result = maximum.result; return output; }
            point.maximumValue = maximum.value;
        }
        const ValueResult value = RegisterValue::fromText(point.dataType, field(QStringLiteral("current_value"))); if (!value.result.success) { output.result = value.result; return output; } point.currentValue = value.value;
        if (headers.contains(QStringLiteral("enabled"))) { point.enabled = field(QStringLiteral("enabled")).trimmed().toLower() != QStringLiteral("false"); }
        if (headers.contains(QStringLiteral("category"))) { point.category = field(QStringLiteral("category")); }
        if (headers.contains(QStringLiteral("label"))) { point.label = field(QStringLiteral("label")); }
        if (headers.contains(QStringLiteral("strategy_type"))) { strategyTypeFromString(field(QStringLiteral("strategy_type")), &point.strategy.type); }
        if (headers.contains(QStringLiteral("strategy_enabled"))) { point.strategy.enabled = field(QStringLiteral("strategy_enabled")).trimmed().toLower() == QStringLiteral("true"); }
        if (headers.contains(QStringLiteral("strategy_params")))
        {
            const QJsonDocument parameters = QJsonDocument::fromJson(field(QStringLiteral("strategy_params")).toUtf8());
            if (parameters.isObject()) { point.strategy.parameters = parameters.object().toVariantMap(); }
        }
        output.registers.append(point);
    }
    ProjectDocument candidate = current; candidate.groups = output.groups; candidate.registers += output.registers;
    output.result = ValidationService::validateProject(candidate); return output;
}

OperationResult CsvRegisterGatewayImpl::exportFile(const QString &path,
                                                   const ProjectDocument &document) const
{
    QSaveFile file(path); if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return OperationResult::fail(QStringLiteral("csv_save_failed"), QStringLiteral("path"), QStringLiteral("无法写入 CSV 文件"), file.errorString()); }
    file.write("\xEF\xBB\xBF"); QTextStream stream(&file); stream.setCodec("UTF-8");
    stream << "group_name,slave_address,address,register_count,name,data_type,endian,storage_type,read_function_code,write_function_code,protocol_key,unit,offset,precision,min_value,max_value,current_value,enabled,category,label,strategy_type,strategy_enabled,strategy_params\n";
    for (const RegisterPoint &point : document.registers)
    {
        QString groupName; for (const RegisterGroup &group : document.groups) { if (group.id == point.groupId) { groupName = group.name; break; } }
        const QString strategyParameters = QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(point.strategy.parameters)).toJson(QJsonDocument::Compact));
        stream << quoteCsv(groupName) << ',' << point.slaveAddress << ',' << point.address << ',' << point.registerCount << ',' << quoteCsv(point.name) << ',' << dataTypeToString(point.dataType) << ',' << endianToString(point.endian) << ',' << storageTypeToString(point.storageType) << ',' << point.readFunctionCode << ',' << point.writeFunctionCode << ',' << quoteCsv(point.protocolKey) << ',' << quoteCsv(point.unit) << ',' << point.offset << ',' << point.precision << ',' << quoteCsv(point.minimumValue.toStorageString()) << ',' << quoteCsv(point.maximumValue.toStorageString()) << ',' << quoteCsv(point.currentValue.toStorageString()) << ',' << (point.enabled ? "true" : "false") << ',' << quoteCsv(point.category) << ',' << quoteCsv(point.label) << ',' << strategyTypeToString(point.strategy.type) << ',' << (point.strategy.enabled ? "true" : "false") << ',' << quoteCsv(strategyParameters) << '\n';
    }
    stream.flush(); if (!file.commit()) { return OperationResult::fail(QStringLiteral("csv_commit_failed"), QStringLiteral("path"), QStringLiteral("无法原子保存 CSV 文件"), file.errorString()); }
    return OperationResult::ok();
}
