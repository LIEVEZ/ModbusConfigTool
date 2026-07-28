#include "json_project_repository.h"

#include "Domain/Models/connection_port.h"
#include "Domain/Models/project_factory.h"
#include "Domain/Validation/validation_service.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

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
    point.enabled = true; // 点位启用字段已废弃，加载后一律生效
    point.category = object.value(QStringLiteral("category")).toString();
    point.label = object.value(QStringLiteral("label")).toString();
    const QJsonObject strategy = object.value(QStringLiteral("strategy")).toObject();
    strategyTypeFromString(strategy.value(QStringLiteral("type")).toString(), &point.strategy.type);
    point.strategy.enabled = strategy.value(QStringLiteral("enabled")).toBool();
    point.strategy.parameters = strategy.value(QStringLiteral("params")).toObject().toVariantMap();
    return point;
}

QJsonObject portToJson(const ConnectionPort &port)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), port.id);
    object.insert(QStringLiteral("name"), port.name);
    object.insert(QStringLiteral("connectionType"),
                  port.profile.connectionType == ConnectionType::Tcp ? QStringLiteral("TCP") : QStringLiteral("RTU"));
    object.insert(QStringLiteral("tcpHost"), port.profile.tcpHost);
    object.insert(QStringLiteral("tcpPort"), int(port.profile.tcpPort));
    object.insert(QStringLiteral("serialPort"), port.profile.serialPort);
    object.insert(QStringLiteral("baudRate"), port.profile.baudRate);
    object.insert(QStringLiteral("parity"), QString(port.profile.parity));
    object.insert(QStringLiteral("dataBits"), port.profile.dataBits);
    object.insert(QStringLiteral("stopBits"), port.profile.stopBits);
    object.insert(QStringLiteral("pollIntervalMs"), port.profile.pollIntervalMs);
    return object;
}

ServerProfile profileFromJson(const QJsonObject &object)
{
    ServerProfile profile;
    profile.connectionType = object.value(QStringLiteral("connectionType")).toString() == QStringLiteral("RTU")
                             ? ConnectionType::Rtu : ConnectionType::Tcp;
    profile.tcpHost = object.value(QStringLiteral("tcpHost")).toString(QStringLiteral("127.0.0.1"));
    profile.tcpPort = quint16(object.value(QStringLiteral("tcpPort")).toInt(5020));
    profile.serialPort = object.value(QStringLiteral("serialPort")).toString();
    profile.baudRate = object.value(QStringLiteral("baudRate")).toInt(9600);
    const QString parity = object.value(QStringLiteral("parity")).toString(QStringLiteral("N"));
    profile.parity = parity.isEmpty() ? QLatin1Char('N') : parity.at(0);
    profile.dataBits = object.value(QStringLiteral("dataBits")).toInt(8);
    profile.stopBits = object.value(QStringLiteral("stopBits")).toInt(1);
    profile.pollIntervalMs = object.value(QStringLiteral("pollIntervalMs")).toInt(1000);
    // 端口不再绑定从站地址；旧工程中的 slaveAddress 字段忽略
    return profile;
}

ConnectionPort portFromJson(const QJsonObject &object)
{
    ConnectionPort port;
    port.id = object.value(QStringLiteral("id")).toString();
    port.name = object.value(QStringLiteral("name")).toString();
    port.profile = profileFromJson(object);
    return port;
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
    const int schemaVersion = root.value(QStringLiteral("schemaVersion")).toInt();
    if (schemaVersion < 1 || schemaVersion > 2)
    {
        output.result = OperationResult::fail(QStringLiteral("unsupported_version"),
                                              QStringLiteral("schemaVersion"),
                                              QStringLiteral("不支持该工程文件版本"));
        return output;
    }

    ProjectDocument document;
    document.schemaVersion = 2;  // 输出始终为 v2
    const QJsonObject metadata = root.value(QStringLiteral("project")).toObject();
    document.project.name = metadata.value(QStringLiteral("name")).toString();
    document.project.description = metadata.value(QStringLiteral("description")).toString();
    document.project.createdAt = QDateTime::fromString(metadata.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    document.project.updatedAt = QDateTime::fromString(metadata.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);

    // v1 迁移：单 serverProfile → 一个默认端口
    if (schemaVersion == 1)
    {
        const QJsonObject profile = root.value(QStringLiteral("serverProfile")).toObject();
        ConnectionPort port;
        port.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        port.name = QStringLiteral("默认端口");
        port.profile = profileFromJson(profile);
        document.ports.append(port);
    }
    else  // v2
    {
        for (const QJsonValue &value : root.value(QStringLiteral("ports")).toArray())
        {
            document.ports.append(portFromJson(value.toObject()));
        }
    }

    for (const QJsonValue &value : root.value(QStringLiteral("groups")).toArray())
    {
        const QJsonObject groupJson = value.toObject();
        RegisterGroup group;
        group.id = groupJson.value(QStringLiteral("id")).toString();
        group.name = groupJson.value(QStringLiteral("name")).toString();
        group.color = groupJson.value(QStringLiteral("color")).toString(QStringLiteral("#f54e00"));
        group.description = groupJson.value(QStringLiteral("description")).toString();
        group.isDefault = groupJson.value(QStringLiteral("isDefault")).toBool();
        group.enabled = groupJson.value(QStringLiteral("enabled")).toBool(true);
        group.canvasX = groupJson.value(QStringLiteral("canvasX")).toInt(0);
        group.canvasY = groupJson.value(QStringLiteral("canvasY")).toInt(0);
        // v1 迁移：所有分组绑定到唯一端口
        if (schemaVersion == 1 && !document.ports.isEmpty())
        {
            group.portId = document.ports.first().id;
        }
        else  // v2
        {
            group.portId = groupJson.value(QStringLiteral("portId")).toString();
        }
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
    document.uiState.portColWidth = uiState.value(QStringLiteral("portColWidth")).toInt(250);
    document.uiState.logColWidth = uiState.value(QStringLiteral("logColWidth")).toInt(300);
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
    root.insert(QStringLiteral("schemaVersion"), 2);
    QJsonObject metadata;
    metadata.insert(QStringLiteral("name"), document.project.name);
    metadata.insert(QStringLiteral("description"), document.project.description);
    metadata.insert(QStringLiteral("createdAt"), document.project.createdAt.toString(Qt::ISODate));
    metadata.insert(QStringLiteral("updatedAt"), document.project.updatedAt.toString(Qt::ISODate));
    root.insert(QStringLiteral("project"), metadata);

    QJsonArray ports;
    for (const ConnectionPort &port : document.ports)
    {
        ports.append(portToJson(port));
    }
    root.insert(QStringLiteral("ports"), ports);

    QJsonArray groups;
    for (const RegisterGroup &group : document.groups)
    {
        QJsonObject object;
        object.insert(QStringLiteral("id"), group.id);
        object.insert(QStringLiteral("name"), group.name);
        object.insert(QStringLiteral("color"), group.color);
        object.insert(QStringLiteral("description"), group.description);
        object.insert(QStringLiteral("isDefault"), group.isDefault);
        object.insert(QStringLiteral("enabled"), group.enabled);
        object.insert(QStringLiteral("portId"), group.portId);
        object.insert(QStringLiteral("canvasX"), group.canvasX);
        object.insert(QStringLiteral("canvasY"), group.canvasY);
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
    uiState.insert(QStringLiteral("portColWidth"), document.uiState.portColWidth);
    uiState.insert(QStringLiteral("logColWidth"), document.uiState.logColWidth);
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
