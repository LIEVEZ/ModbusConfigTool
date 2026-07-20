#ifndef CONNECTION_SERVICE_H
#define CONNECTION_SERVICE_H

#include "Application/Project/project_service.h"
#include "Domain/Models/connection_port.h"

class ConnectionService
{
public:
    explicit ConnectionService(ProjectService *projectService);

    OperationResult addPort(const ConnectionPort &port);
    OperationResult updatePort(const ConnectionPort &port);
    OperationResult removePort(const QString &portId);
    ConnectionPort makeDefaultPort() const;

private:
    ProjectService *m_projectService = nullptr;
};

#endif
