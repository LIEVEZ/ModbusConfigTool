#ifndef MODBUS_RUNTIME_WORKER_H
#define MODBUS_RUNTIME_WORKER_H

#include "Domain/Models/project_document.h"

#include <QObject>

class QModbusServer;
class StrategyEngine;

class ModbusRuntimeWorker : public QObject
{
    Q_OBJECT

public:
    explicit ModbusRuntimeWorker(QObject *parent = nullptr);

public slots:
    void start(const ServerProfile &profile, const QList<RegisterPoint> &points);
    void stop();
    void writePoint(const QString &pointId, const RegisterValue &value);

signals:
    void started();
    void stopped();
    void failed(const QString &message, const QString &detail);
    void valueChanged(const QString &pointId, const RegisterValue &value);

private:
    QModbusServer *m_server = nullptr;
    StrategyEngine *m_strategyEngine = nullptr;
    QHash<QString, RegisterPoint> m_points;
};

Q_DECLARE_METATYPE(ServerProfile)
Q_DECLARE_METATYPE(RegisterPoint)

#endif
