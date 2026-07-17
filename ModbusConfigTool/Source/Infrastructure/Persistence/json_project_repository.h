#ifndef JSON_PROJECT_REPOSITORY_H
#define JSON_PROJECT_REPOSITORY_H

#include "Domain/Interfaces/project_repository.h"

class JsonProjectRepository : public ProjectRepository
{
public:
    ProjectLoadResult load(const QString &path) const override;
    OperationResult save(const QString &path,
                         const ProjectDocument &document) const override;
};

#endif
