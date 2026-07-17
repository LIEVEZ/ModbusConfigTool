#ifndef PROJECT_FACTORY_H
#define PROJECT_FACTORY_H

#include "Domain/Models/project_document.h"

class ProjectFactory
{
public:
    static ProjectDocument createEmpty();
    static RegisterPoint createRegister(const QString &groupId, quint16 address = 0);
    static quint16 registerCountFor(DataType type);
    static RegisterValue minimumFor(DataType type);
    static RegisterValue maximumFor(DataType type);
};

#endif
