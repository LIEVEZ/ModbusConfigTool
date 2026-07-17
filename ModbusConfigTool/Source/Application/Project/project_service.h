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
    QStringList recentFiles() const;

    void newProject();
    OperationResult open(const QString &path);
    OperationResult save();
    OperationResult saveAs(const QString &path);
    void markDirty();
    void updateRuntimeValue(const QString &pointId, const RegisterValue &value);
    void removeRecentFile(const QString &path);

signals:
    void documentChanged();
    void dirtyChanged(bool dirty);
    void recentFilesChanged(const QStringList &paths);
    void runtimeValueChanged(const QString &pointId);

private:
    void setDirty(bool dirty);
    void addRecentFile(const QString &path);

    ProjectRepository *m_repository = nullptr;
    ProjectDocument m_document;
    QString m_filePath;
    bool m_dirty = false;
    QStringList m_recentFiles;
};

#endif
