#ifndef PROJECT_REPOSITORY_H
#define PROJECT_REPOSITORY_H

#include "Domain/Models/project_document.h"
#include "Domain/Values/operation_result.h"

struct ProjectLoadResult
{
    OperationResult result;
    ProjectDocument document;
};

class ProjectRepository
{
public:
    virtual ~ProjectRepository() = default;
    virtual ProjectLoadResult load(const QString &path) const = 0;
    virtual OperationResult save(const QString &path,
                                 const ProjectDocument &document) const = 0;
};

#endif
