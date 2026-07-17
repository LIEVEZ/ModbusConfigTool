#include "project_service.h"

#include "Domain/Models/project_factory.h"

ProjectService::ProjectService(ProjectRepository *repository, QObject *parent)
    : QObject(parent),
      m_repository(repository),
      m_document(ProjectFactory::createEmpty())
{
}

const ProjectDocument &ProjectService::document() const { return m_document; }
ProjectDocument &ProjectService::editableDocument() { return m_document; }
QString ProjectService::filePath() const { return m_filePath; }
bool ProjectService::isDirty() const { return m_dirty; }

void ProjectService::newProject()
{
    m_document = ProjectFactory::createEmpty();
    m_filePath.clear();
    setDirty(false);
    emit documentChanged();
}

OperationResult ProjectService::open(const QString &path)
{
    const ProjectLoadResult loaded = m_repository->load(path);
    if (!loaded.result.success) { return loaded.result; }
    m_document = loaded.document;
    m_filePath = path;
    setDirty(false);
    emit documentChanged();
    return OperationResult::ok();
}

OperationResult ProjectService::save()
{
    if (m_filePath.isEmpty())
    {
        return OperationResult::fail(QStringLiteral("path_required"), QStringLiteral("path"),
                                     QStringLiteral("请先选择工程保存路径"));
    }
    return saveAs(m_filePath);
}

OperationResult ProjectService::saveAs(const QString &path)
{
    m_document.project.updatedAt = QDateTime::currentDateTime();
    const OperationResult result = m_repository->save(path, m_document);
    if (result.success)
    {
        m_filePath = path;
        setDirty(false);
    }
    return result;
}

void ProjectService::markDirty()
{
    setDirty(true);
    emit documentChanged();
}

void ProjectService::setDirty(bool dirty)
{
    if (m_dirty == dirty) { return; }
    m_dirty = dirty;
    emit dirtyChanged(m_dirty);
}
