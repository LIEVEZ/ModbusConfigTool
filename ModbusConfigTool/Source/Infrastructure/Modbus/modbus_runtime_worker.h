#ifndef MODBUS_RUNTIME_WORKER_H
#define MODBUS_RUNTIME_WORKER_H

#include "Domain/Models/project_document.h"

#include <QObject>

class QModbusServer;

class ModbusRuntimeWorker : public QObject
{
    Q_OBJECT

public:
    explicit ModbusRuntimeWorker(QObject *parent = nullptr);

public slots:
    void start(const ServerProfile &profile, const QList<RegisterPoint> &points);
    void stop();

signals:
    void started();
    void stopped();
    void failed(const QString &message, const QString &detail);

private:
    QModbusServer *m_server = nullptr;
};

Q_DECLARE_METATYPE(ServerProfile)
Q_DECLARE_METATYPE(RegisterPoint)

#endif
