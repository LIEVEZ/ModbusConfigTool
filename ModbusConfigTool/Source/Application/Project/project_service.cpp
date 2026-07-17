#include "project_service.h"

#include "Domain/Models/project_factory.h"

#include <QFileInfo>
#include <QSettings>

ProjectService::ProjectService(ProjectRepository *repository, QObject *parent)
    : QObject(parent),
      m_repository(repository),
      m_document(ProjectFactory::createEmpty())
{
    QSettings settings;
    m_recentFiles = settings.value(QStringLiteral("recentProjects")).toStringList();
}

const ProjectDocument &ProjectService::document() const { return m_document; }
ProjectDocument &ProjectService::editableDocument() { return m_document; }
QString ProjectService::filePath() const { return m_filePath; }
bool ProjectService::isDirty() const { return m_dirty; }
QStringList ProjectService::recentFiles() const { return m_recentFiles; }

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
    addRecentFile(path);
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
        addRecentFile(path);
        setDirty(false);
    }
    return result;
}

void ProjectService::addRecentFile(const QString &path)
{
    const QString normalized = QFileInfo(path).absoluteFilePath();
    m_recentFiles.removeAll(normalized);
    m_recentFiles.prepend(normalized);
    while (m_recentFiles.size() > 10) { m_recentFiles.removeLast(); }
    QSettings settings;
    settings.setValue(QStringLiteral("recentProjects"), m_recentFiles);
    emit recentFilesChanged(m_recentFiles);
}

void ProjectService::removeRecentFile(const QString &path)
{
    m_recentFiles.removeAll(QFileInfo(path).absoluteFilePath());
    QSettings settings;
    settings.setValue(QStringLiteral("recentProjects"), m_recentFiles);
    emit recentFilesChanged(m_recentFiles);
}

void ProjectService::markDirty()
{
    setDirty(true);
    emit documentChanged();
}

void ProjectService::updateRuntimeValue(const QString &pointId,
                                        const RegisterValue &value)
{
    for (RegisterPoint &point : m_document.registers)
    {
        if (point.id == pointId)
        {
            point.currentValue = value;
            emit runtimeValueChanged(pointId);
            return;
        }
    }
}

void ProjectService::setDirty(bool dirty)
{
    if (m_dirty == dirty) { return; }
    m_dirty = dirty;
    emit dirtyChanged(m_dirty);
}
