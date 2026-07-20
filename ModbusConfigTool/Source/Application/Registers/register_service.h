#ifndef REGISTER_SERVICE_H
#define REGISTER_SERVICE_H

#include "Application/Project/project_service.h"
#include "Application/Registers/register_patch.h"
#include "Domain/Interfaces/csv_register_gateway.h"

class RegisterService
{
public:
    explicit RegisterService(ProjectService *projectService);

    OperationResult addGroup(const QString &name);
    OperationResult addGroup(const RegisterGroup &group);
    OperationResult updateGroup(const RegisterGroup &group);
    OperationResult setGroupEnabled(const QString &groupId, bool enabled);
    OperationResult setGroupPort(const QString &groupId, const QString &portId);
    OperationResult moveGroup(const QString &groupId, int x, int y);
    OperationResult removeGroup(const QString &groupId, bool removePoints);
    quint16 nextAddress(const QString &groupId) const;
    OperationResult addRegister(const RegisterPoint &point);
    OperationResult updateRegister(const RegisterPoint &point);
    OperationResult removeRegisters(const QStringList &ids);
    OperationResult setEnabled(const QStringList &ids, bool enabled);
    OperationResult applyPatch(const QStringList &ids, const RegisterPatch &patch);
    OperationResult importCsvIntoGroup(const QString &groupId,
                                       const CsvImportResult &imported,
                                       bool replaceGroup);

private:
    ProjectService *m_projectService = nullptr;
};

#endif
