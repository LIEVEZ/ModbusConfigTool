#include "connection_service.h"

#include "Domain/Validation/validation_service.h"

#include <QUuid>

ConnectionService::ConnectionService(ProjectService *projectService)
    : m_projectService(projectService)
{
}

ConnectionPort ConnectionService::makeDefaultPort() const
{
    ConnectionPort port;
    port.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    port.name = QStringLiteral("新端口");
    return port;
}

OperationResult ConnectionService::addPort(const ConnectionPort &port)
{
    ProjectDocument candidate = m_projectService->document();
    ConnectionPort added = port;
    if (added.id.isEmpty())
    {
        added.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    candidate.ports.append(added);
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult ConnectionService::updatePort(const ConnectionPort &port)
{
    ProjectDocument candidate = m_projectService->document();
    bool found = false;
    for (ConnectionPort &current : candidate.ports)
    {
        if (current.id == port.id) { current = port; found = true; break; }
    }
    if (!found)
    {
        return OperationResult::fail(QStringLiteral("missing_port"),
                                     QStringLiteral("id"),
                                     QStringLiteral("连接端口不存在"));
    }
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult ConnectionService::removePort(const QString &portId)
{
    ProjectDocument candidate = m_projectService->document();
    int index = -1;
    for (int i = 0; i < candidate.ports.size(); ++i)
    {
        if (candidate.ports.at(i).id == portId) { index = i; break; }
    }
    if (index < 0)
    {
        return OperationResult::fail(QStringLiteral("missing_port"),
                                     QStringLiteral("portId"),
                                     QStringLiteral("连接端口不存在"));
    }
    for (RegisterGroup &group : candidate.groups)
    {
        if (group.portId == portId) { group.portId.clear(); }
    }
    candidate.ports.removeAt(index);
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}
