#include "project_service.h"

#include "Domain/Models/project_factory.h"

#include <QFileInfo>
#include <QHash>
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
    // 旧工程可能仍是默认名“未命名工程”：打开时用文件名纠正显示/后续保存
    const QString fileTitle = QFileInfo(path).completeBaseName().trimmed();
    if (!fileTitle.isEmpty()
        && (m_document.project.name.trimmed().isEmpty()
            || m_document.project.name.trimmed() == QStringLiteral("未命名工程")))
    {
        m_document.project.name = fileTitle;
    }
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
    // 另存为时若仍是默认名，同步为文件名（不含扩展名）
    const QString fileTitle = QFileInfo(path).completeBaseName().trimmed();
    if (!fileTitle.isEmpty()
        && (m_document.project.name.trimmed().isEmpty()
            || m_document.project.name.trimmed() == QStringLiteral("未命名工程")))
    {
        m_document.project.name = fileTitle;
    }
    m_document.project.updatedAt = QDateTime::currentDateTime();
    const OperationResult result = m_repository->save(path, m_document);
    if (result.success)
    {
        m_filePath = path;
        addRecentFile(path);
        setDirty(false);
        emit documentChanged();
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
            // 仅标记脏，不发 documentChanged，避免整窗体重刷
            setDirty(true);
            emit runtimeValueChanged(pointId);
            return;
        }
    }
}

void ProjectService::updateRuntimeValues(const QList<QPair<QString, RegisterValue>> &values)
{
    if (values.isEmpty())
    {
        return;
    }

    QHash<QString, RegisterValue> valueMap;
    valueMap.reserve(values.size());
    for (const auto &item : values)
    {
        valueMap.insert(item.first, item.second);
    }

    bool changed = false;
    for (RegisterPoint &point : m_document.registers)
    {
        const auto it = valueMap.constFind(point.id);
        if (it == valueMap.cend())
        {
            continue;
        }
        point.currentValue = it.value();
        changed = true;
    }

    if (!changed)
    {
        return;
    }

    setDirty(true);
    // 空 id 表示批量刷新
    emit runtimeValueChanged(QString());
}

void ProjectService::setDirty(bool dirty)
{
    if (m_dirty == dirty) { return; }
    m_dirty = dirty;
    emit dirtyChanged(m_dirty);
}
