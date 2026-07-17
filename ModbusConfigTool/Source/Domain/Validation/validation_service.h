#ifndef VALIDATION_SERVICE_H
#define VALIDATION_SERVICE_H

#include "Domain/Models/project_document.h"
#include "Domain/Values/operation_result.h"

class ValidationService
{
public:
    static OperationResult validateServerProfile(const ServerProfile &profile);
    static OperationResult validateRegister(const RegisterPoint &point);
    static OperationResult validateProject(const ProjectDocument &document);
};

#endif
