#ifndef MODBUS_RUNTIME_WORKER_H
#define MODBUS_RUNTIME_WORKER_H

#include "Domain/Models/comm_frame.h"
#include "Domain/Models/project_document.h"
#include "Infrastructure/Modbus/modbus_register_store.h"

#include <QModbusDataUnit>
#include <QObject>

class MultiSlaveModbusServer;
class StrategyEngine;

class ModbusRuntimeWorker : public QObject
{
    Q_OBJECT

public:
    explicit ModbusRuntimeWorker(QObject *parent = nullptr);

public slots:
    void start(const ServerProfile &profile, const QList<RegisterPoint> &points);
    void reloadPoints(const QList<RegisterPoint> &points);
    void stop();
    void writePoint(const QString &pointId, const RegisterValue &value);

signals:
    void started();
    void stopped();
    void failed(const QString &message, const QString &detail);
    void valueChanged(const QString &pointId, const RegisterValue &value);
    void diagnostics(const QString &message);
    void frameCaptured(const CommFrame &frame);

private:
    void handleDataWritten(quint8 slaveAddress, QModbusDataUnit::RegisterType table, int address, int size);
    void rebuildMap(const QList<RegisterPoint> &points, bool restartStrategy);

    MultiSlaveModbusServer *m_server = nullptr;
    StrategyEngine *m_strategyEngine = nullptr;
    ServerProfile m_profile;
    QHash<QString, RegisterPoint> m_points;
    ModbusRegisterStore m_store;
};

Q_DECLARE_METATYPE(ServerProfile)
Q_DECLARE_METATYPE(RegisterPoint)
Q_DECLARE_METATYPE(QList<RegisterPoint>)

#endif
