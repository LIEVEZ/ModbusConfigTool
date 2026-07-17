#ifndef CSV_REGISTER_GATEWAY_IMPL_H
#define CSV_REGISTER_GATEWAY_IMPL_H

#include "Domain/Interfaces/csv_register_gateway.h"

class CsvRegisterGatewayImpl : public CsvRegisterGateway
{
public:
    CsvImportResult importFile(const QString &path,
                               const ProjectDocument &current) const override;
    OperationResult exportFile(const QString &path,
                               const ProjectDocument &document) const override;
};

#endif
