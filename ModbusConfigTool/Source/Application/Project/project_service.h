#ifndef PROJECT_SERVICE_H
#define PROJECT_SERVICE_H

#include "Domain/Interfaces/project_repository.h"

#include <QObject>

class ProjectService : public QObject
{
    Q_OBJECT

public:
    explicit ProjectService(ProjectRepository *repository, QObject *parent = nullptr);

    const ProjectDocument &document() const;
    ProjectDocument &editableDocument();
    QString filePath() const;
    bool isDirty() const;

    void newProject();
    OperationResult open(const QString &path);
    OperationResult save();
    OperationResult saveAs(const QString &path);
    void markDirty();

signals:
    void documentChanged();
    void dirtyChanged(bool dirty);

private:
    void setDirty(bool dirty);

    ProjectRepository *m_repository = nullptr;
    ProjectDocument m_document;
    QString m_filePath;
    bool m_dirty = false;
};

#endif
