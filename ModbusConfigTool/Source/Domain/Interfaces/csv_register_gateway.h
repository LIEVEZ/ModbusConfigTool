#ifndef CSV_REGISTER_GATEWAY_H
#define CSV_REGISTER_GATEWAY_H

#include "Domain/Models/project_document.h"
#include "Domain/Values/operation_result.h"

struct CsvImportResult
{
    OperationResult result;
    QList<RegisterGroup> groups;
    QList<RegisterPoint> registers;
};

class CsvRegisterGateway
{
public:
    virtual ~CsvRegisterGateway() = default;
    virtual CsvImportResult importFile(const QString &path,
                                       const ProjectDocument &current) const = 0;
    virtual OperationResult exportFile(const QString &path,
                                       const ProjectDocument &document) const = 0;
};

#endif
