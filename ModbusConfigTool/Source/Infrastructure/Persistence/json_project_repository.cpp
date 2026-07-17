#include "json_project_repository.h"

#include "Domain/Models/project_factory.h"
#include "Domain/Validation/validation_service.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace
{
QJsonObject valueToJson(const RegisterValue &value)
{
    QJsonObject object;
    object.insert(QStringLiteral("type"), dataTypeToString(value.dataType()));
    if (value.dataType() == DataType::Int64 || value.dataType() == DataType::UInt64)
    {
        object.insert(QStringLiteral("value"), value.toStorageString());
    }
    else
    {
        object.insert(QStringLiteral("value"), value.toDouble());
    }
    return object;
}

ValueResult valueFromJson(const QJsonValue &jsonValue, DataType fallbackType)
{
    if (!jsonValue.isObject())
    {
        return RegisterValue::fromText(fallbackType, jsonValue.toVariant().toString());
    }
    const QJsonObject object = jsonValue.toObject();
    DataType type = fallbackType;
    dataTypeFromString(object.value(QStringLiteral("type")).toString(), &type);
    return RegisterValue::fromText(type, object.value(QStringLiteral("value")).toVariant().toString());
}

QJsonObject pointToJson(const RegisterPoint &point)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), point.id);
    object.insert(QStringLiteral("groupId"), point.groupId);
    object.insert(QStringLiteral("slaveAddress"), int(point.slaveAddress));
    object.insert(QStringLiteral("address"), int(point.address));
    object.insert(QStringLiteral("registerCount"), int(point.registerCount));
    object.insert(QStringLiteral("name"), point.name);
    object.insert(QStringLiteral("dataType"), dataTypeToString(point.dataType));
    object.insert(QStringLiteral("endian"), endianToString(point.endian));
    object.insert(QStringLiteral("storageType"), storageTypeToString(point.storageType));
    object.insert(QStringLiteral("readFunctionCode"), int(point.readFunctionCode));
    object.insert(QStringLiteral("writeFunctionCode"), point.writeFunctionCode);
    object.insert(QStringLiteral("protocolKey"), point.protocolKey);
    object.insert(QStringLiteral("unit"), point.unit);
    object.insert(QStringLiteral("offset"), point.offset);
    object.insert(QStringLiteral("precision"), point.precision);
    object.insert(QStringLiteral("minValue"), valueToJson(point.minimumValue));
    object.insert(QStringLiteral("maxValue"), valueToJson(point.maximumValue));
    object.insert(QStringLiteral("currentValue"), valueToJson(point.currentValue));
    object.insert(QStringLiteral("enabled"), point.enabled);
    object.insert(QStringLiteral("category"), point.category);
    object.insert(QStringLiteral("label"), point.label);
    QJsonObject strategy;
    strategy.insert(QStringLiteral("type"), strategyTypeToString(point.strategy.type));
    strategy.insert(QStringLiteral("enabled"), point.strategy.enabled);
    strategy.insert(QStringLiteral("params"), QJsonObject::fromVariantMap(point.strategy.parameters));
    object.insert(QStringLiteral("strategy"), strategy);
    return object;
}

RegisterPoint pointFromJson(const QJsonObject &object)
{
    RegisterPoint point;
    point.id = object.value(QStringLiteral("id")).toString();
    point.groupId = object.value(QStringLiteral("groupId")).toString();
    point.slaveAddress = quint8(object.value(QStringLiteral("slaveAddress")).toInt(1));
    point.address = quint16(object.value(QStringLiteral("address")).toInt());
    point.registerCount = quint16(object.value(QStringLiteral("registerCount")).toInt(1));
    point.name = object.value(QStringLiteral("name")).toString();
    dataTypeFromString(object.value(QStringLiteral("dataType")).toString(), &point.dataType);
    endianFromString(object.value(QStringLiteral("endian")).toString(), &point.endian);
    storageTypeFromString(object.value(QStringLiteral("storageType")).toString(), &point.storageType);
    point.readFunctionCode = quint8(object.value(QStringLiteral("readFunctionCode")).toInt(3));
    point.writeFunctionCode = object.value(QStringLiteral("writeFunctionCode")).toInt(6);
    point.protocolKey = object.value(QStringLiteral("protocolKey")).toString();
    point.unit = object.value(QStringLiteral("unit")).toString();
    point.offset = object.value(QStringLiteral("offset")).toDouble();
    point.precision = object.value(QStringLiteral("precision")).toInt();
    point.minimumValue = valueFromJson(object.value(QStringLiteral("minValue")), point.dataType).value;
    point.maximumValue = valueFromJson(object.value(QStringLiteral("maxValue")), point.dataType).value;
    point.currentValue = valueFromJson(object.value(QStringLiteral("currentValue")), point.dataType).value;
    point.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    point.category = object.value(QStringLiteral("category")).toString();
    point.label = object.value(QStringLiteral("label")).toString();
    const QJsonObject strategy = object.value(QStringLiteral("strategy")).toObject();
    strategyTypeFromString(strategy.value(QStringLiteral("type")).toString(), &point.strategy.type);
    point.strategy.enabled = strategy.value(QStringLiteral("enabled")).toBool();
    point.strategy.parameters = strategy.value(QStringLiteral("params")).toObject().toVariantMap();
    return point;
}
}

ProjectLoadResult JsonProjectRepository::load(const QString &path) const
{
    ProjectLoadResult output;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        output.result = OperationResult::fail(QStringLiteral("open_failed"),
                                              QStringLiteral("path"),
                                              QStringLiteral("无法打开工程文件"), file.errorString());
        return output;
    }
    QJsonParseError error;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !json.isObject())
    {
        output.result = OperationResult::fail(QStringLiteral("invalid_json"),
                                              QStringLiteral("file"),
                                              QStringLiteral("工程文件不是有效 JSON"), error.errorString());
        return output;
    }
    const QJsonObject root = json.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt() != 1)
    {
        output.result = OperationResult::fail(QStringLiteral("unsupported_version"),
                                              QStringLiteral("schemaVersion"),
                                              QStringLiteral("不支持该工程文件版本"));
        return output;
    }

    ProjectDocument document;
    const QJsonObject metadata = root.value(QStringLiteral("project")).toObject();
    document.project.name = metadata.value(QStringLiteral("name")).toString();
    document.project.description = metadata.value(QStringLiteral("description")).toString();
    document.project.createdAt = QDateTime::fromString(metadata.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    document.project.updatedAt = QDateTime::fromString(metadata.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);
    const QJsonObject profile = root.value(QStringLiteral("serverProfile")).toObject();
    document.serverProfile.connectionType = profile.value(QStringLiteral("connectionType")).toString() == QStringLiteral("RTU") ? ConnectionType::Rtu : ConnectionType::Tcp;
    document.serverProfile.tcpHost = profile.value(QStringLiteral("tcpHost")).toString(QStringLiteral("127.0.0.1"));
    document.serverProfile.tcpPort = quint16(profile.value(QStringLiteral("tcpPort")).toInt(5020));
    document.serverProfile.serialPort = profile.value(QStringLiteral("serialPort")).toString();
    document.serverProfile.baudRate = profile.value(QStringLiteral("baudRate")).toInt(9600);
    const QString parity = profile.value(QStringLiteral("parity")).toString(QStringLiteral("N"));
    document.serverProfile.parity = parity.isEmpty() ? QLatin1Char('N') : parity.at(0);
    document.serverProfile.dataBits = profile.value(QStringLiteral("dataBits")).toInt(8);
    document.serverProfile.stopBits = profile.value(QStringLiteral("stopBits")).toInt(1);
    document.serverProfile.pollIntervalMs = profile.value(QStringLiteral("pollIntervalMs")).toInt(1000);
    document.serverProfile.slaveAddress = quint8(profile.value(QStringLiteral("slaveAddress")).toInt(1));
    for (const QJsonValue &value : root.value(QStringLiteral("groups")).toArray())
    {
        const QJsonObject groupJson = value.toObject();
        RegisterGroup group;
        group.id = groupJson.value(QStringLiteral("id")).toString();
        group.name = groupJson.value(QStringLiteral("name")).toString();
        group.color = groupJson.value(QStringLiteral("color")).toString(QStringLiteral("#f54e00"));
        group.description = groupJson.value(QStringLiteral("description")).toString();
        group.isDefault = groupJson.value(QStringLiteral("isDefault")).toBool();
        document.groups.append(group);
    }
    for (const QJsonValue &value : root.value(QStringLiteral("registers")).toArray())
    {
        document.registers.append(pointFromJson(value.toObject()));
    }
    const QJsonObject uiState = root.value(QStringLiteral("uiState")).toObject();
    document.uiState.windowSize = QSize(uiState.value(QStringLiteral("width")).toInt(1440),
                                        uiState.value(QStringLiteral("height")).toInt(900));
    document.uiState.selectedGroupId = uiState.value(QStringLiteral("selectedGroupId")).toString();
    for (const QJsonValue &value : uiState.value(QStringLiteral("horizontalSplitterSizes")).toArray())
    {
        document.uiState.horizontalSplitterSizes.append(value.toInt());
    }
    for (const QJsonValue &value : uiState.value(QStringLiteral("verticalSplitterSizes")).toArray())
    {
        document.uiState.verticalSplitterSizes.append(value.toInt());
    }
    output.result = ValidationService::validateProject(document);
    if (output.result.success) { output.document = document; }
    return output;
}

OperationResult JsonProjectRepository::save(const QString &path,
                                            const ProjectDocument &document) const
{
    const OperationResult validation = ValidationService::validateProject(document);
    if (!validation.success) { return validation; }
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    QJsonObject metadata;
    metadata.insert(QStringLiteral("name"), document.project.name);
    metadata.insert(QStringLiteral("description"), document.project.description);
    metadata.insert(QStringLiteral("createdAt"), document.project.createdAt.toString(Qt::ISODate));
    metadata.insert(QStringLiteral("updatedAt"), document.project.updatedAt.toString(Qt::ISODate));
    root.insert(QStringLiteral("project"), metadata);
    QJsonObject profile;
    profile.insert(QStringLiteral("connectionType"), document.serverProfile.connectionType == ConnectionType::Tcp ? QStringLiteral("TCP") : QStringLiteral("RTU"));
    profile.insert(QStringLiteral("tcpHost"), document.serverProfile.tcpHost);
    profile.insert(QStringLiteral("tcpPort"), int(document.serverProfile.tcpPort));
    profile.insert(QStringLiteral("serialPort"), document.serverProfile.serialPort);
    profile.insert(QStringLiteral("baudRate"), document.serverProfile.baudRate);
    profile.insert(QStringLiteral("parity"), QString(document.serverProfile.parity));
    profile.insert(QStringLiteral("dataBits"), document.serverProfile.dataBits);
    profile.insert(QStringLiteral("stopBits"), document.serverProfile.stopBits);
    profile.insert(QStringLiteral("pollIntervalMs"), document.serverProfile.pollIntervalMs);
    profile.insert(QStringLiteral("slaveAddress"), int(document.serverProfile.slaveAddress));
    root.insert(QStringLiteral("serverProfile"), profile);
    QJsonArray groups;
    for (const RegisterGroup &group : document.groups)
    {
        QJsonObject object;
        object.insert(QStringLiteral("id"), group.id);
        object.insert(QStringLiteral("name"), group.name);
        object.insert(QStringLiteral("color"), group.color);
        object.insert(QStringLiteral("description"), group.description);
        object.insert(QStringLiteral("isDefault"), group.isDefault);
        groups.append(object);
    }
    root.insert(QStringLiteral("groups"), groups);
    QJsonArray registers;
    for (const RegisterPoint &point : document.registers) { registers.append(pointToJson(point)); }
    root.insert(QStringLiteral("registers"), registers);
    QJsonObject uiState;
    uiState.insert(QStringLiteral("width"), document.uiState.windowSize.width());
    uiState.insert(QStringLiteral("height"), document.uiState.windowSize.height());
    uiState.insert(QStringLiteral("selectedGroupId"), document.uiState.selectedGroupId);
    QJsonArray horizontalSizes;
    for (int size : document.uiState.horizontalSplitterSizes) { horizontalSizes.append(size); }
    QJsonArray verticalSizes;
    for (int size : document.uiState.verticalSplitterSizes) { verticalSizes.append(size); }
    uiState.insert(QStringLiteral("horizontalSplitterSizes"), horizontalSizes);
    uiState.insert(QStringLiteral("verticalSplitterSizes"), verticalSizes);
    root.insert(QStringLiteral("uiState"), uiState);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return OperationResult::fail(QStringLiteral("save_failed"), QStringLiteral("path"),
                                     QStringLiteral("无法写入工程文件"), file.errorString());
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        return OperationResult::fail(QStringLiteral("commit_failed"), QStringLiteral("path"),
                                     QStringLiteral("无法原子保存工程文件"), file.errorString());
    }
    return OperationResult::ok();
}
