#ifndef REGISTER_SERVICE_H
#define REGISTER_SERVICE_H

#include "Application/Project/project_service.h"
#include "Application/Registers/register_patch.h"

class RegisterService
{
public:
    explicit RegisterService(ProjectService *projectService);

    OperationResult addGroup(const QString &name);
    OperationResult removeGroup(const QString &groupId, bool removePoints);
    quint16 nextAddress(const QString &groupId) const;
    OperationResult addRegister(const RegisterPoint &point);
    OperationResult updateRegister(const RegisterPoint &point);
    OperationResult removeRegisters(const QStringList &ids);
    OperationResult setEnabled(const QStringList &ids, bool enabled);
    OperationResult applyPatch(const QStringList &ids, const RegisterPatch &patch);

private:
    ProjectService *m_projectService = nullptr;
};

#endif
