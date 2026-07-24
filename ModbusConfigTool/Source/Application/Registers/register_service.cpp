#include "register_service.h"

#include "Domain/Validation/validation_service.h"

#include <QUuid>

RegisterService::RegisterService(ProjectService *projectService)
    : m_projectService(projectService)
{
}

OperationResult RegisterService::addGroup(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
    {
        return OperationResult::fail(QStringLiteral("empty_group"), QStringLiteral("name"),
                                     QStringLiteral("分组名称不能为空"));
    }
    ProjectDocument candidate = m_projectService->document();
    for (const RegisterGroup &group : candidate.groups)
    {
        if (group.name == trimmed)
        {
            return OperationResult::fail(QStringLiteral("duplicate_group"), QStringLiteral("name"),
                                         QStringLiteral("分组名称不允许重复"));
        }
    }
    RegisterGroup group;
    group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    group.name = trimmed;
    candidate.groups.append(group);
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::removeGroup(const QString &groupId, bool removePoints)
{
    ProjectDocument candidate = m_projectService->document();
    int groupIndex = -1;
    QString defaultId;
    for (int index = 0; index < candidate.groups.size(); ++index)
    {
        if (candidate.groups.at(index).isDefault)
        {
            defaultId = candidate.groups.at(index).id;
        }
        if (candidate.groups.at(index).id == groupId)
        {
            groupIndex = index;
        }
    }
    if (groupIndex < 0)
    {
        return OperationResult::fail(QStringLiteral("missing_group"),
                                     QStringLiteral("groupId"),
                                     QStringLiteral("分组不存在"));
    }

    const bool removingDefault = candidate.groups.at(groupIndex).isDefault;

    // 寄存器迁移目标：优先其他默认组，否则第一个其余分组；没有则删除寄存器
    QString fallbackGroupId;
    if (!defaultId.isEmpty() && defaultId != groupId)
    {
        fallbackGroupId = defaultId;
    }
    else
    {
        for (const RegisterGroup &group : candidate.groups)
        {
            if (group.id != groupId)
            {
                fallbackGroupId = group.id;
                break;
            }
        }
    }

    for (int index = candidate.registers.size() - 1; index >= 0; --index)
    {
        if (candidate.registers.at(index).groupId != groupId)
        {
            continue;
        }
        if (removePoints || fallbackGroupId.isEmpty())
        {
            candidate.registers.removeAt(index);
        }
        else
        {
            candidate.registers[index].groupId = fallbackGroupId;
        }
    }

    candidate.groups.removeAt(groupIndex);

    // 删除默认组后，若还有其他组则补一个默认标记
    if (removingDefault && !candidate.groups.isEmpty())
    {
        bool hasDefault = false;
        for (const RegisterGroup &group : candidate.groups)
        {
            if (group.isDefault)
            {
                hasDefault = true;
                break;
            }
        }
        if (!hasDefault)
        {
            candidate.groups[0].isDefault = true;
        }
    }

    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

quint16 RegisterService::nextAddress(const QString &groupId) const
{
    quint32 next = 0;
    for (const RegisterPoint &point : m_projectService->document().registers)
    {
        if (point.groupId == groupId)
        {
            next = qMax(next, quint32(point.address) + point.registerCount);
        }
    }
    return quint16(qMin(next, quint32(65535)));
}

OperationResult RegisterService::addRegister(const RegisterPoint &point)
{
    ProjectDocument candidate = m_projectService->document();
    candidate.registers.append(point);
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::updateRegister(const RegisterPoint &point)
{
    ProjectDocument candidate = m_projectService->document();
    bool found = false;
    for (RegisterPoint &current : candidate.registers)
    {
        if (current.id == point.id) { current = point; found = true; break; }
    }
    if (!found) { return OperationResult::fail(QStringLiteral("missing_register"), QStringLiteral("id"), QStringLiteral("寄存器不存在")); }
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::removeRegisters(const QStringList &ids)
{
    ProjectDocument &document = m_projectService->editableDocument();
    for (int index = document.registers.size() - 1; index >= 0; --index)
    {
        if (ids.contains(document.registers.at(index).id)) { document.registers.removeAt(index); }
    }
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::setEnabled(const QStringList &ids, bool enabled)
{
    RegisterPatch patch;
    patch.changeEnabled = true;
    patch.enabled = enabled;
    return applyPatch(ids, patch);
}

OperationResult RegisterService::applyPatch(const QStringList &ids, const RegisterPatch &patch)
{
    ProjectDocument candidate = m_projectService->document();
    for (RegisterPoint &point : candidate.registers)
    {
        if (!ids.contains(point.id)) { continue; }
        if (patch.changeGroup) { point.groupId = patch.groupId; }
        if (patch.changeCategory) { point.category = patch.category; }
        if (patch.changeLabel) { point.label = patch.label; }
        if (patch.changeEnabled) { point.enabled = patch.enabled; }
        if (patch.changeStrategy) { point.strategy = patch.strategy; }
    }
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::addGroup(const RegisterGroup &group)
{
    ProjectDocument candidate = m_projectService->document();
    RegisterGroup added = group;
    if (added.id.isEmpty())
    {
        added.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    candidate.groups.append(added);
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::updateGroup(const RegisterGroup &group)
{
    ProjectDocument candidate = m_projectService->document();
    bool found = false;
    for (RegisterGroup &current : candidate.groups)
    {
        if (current.id == group.id) { current = group; found = true; break; }
    }
    if (!found)
    {
        return OperationResult::fail(QStringLiteral("missing_group"),
                                     QStringLiteral("id"),
                                     QStringLiteral("分组不存在"));
    }
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::setGroupEnabled(const QString &groupId, bool enabled)
{
    ProjectDocument candidate = m_projectService->document();
    for (RegisterGroup &group : candidate.groups)
    {
        if (group.id == groupId) { group.enabled = enabled; break; }
    }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::setGroupPort(const QString &groupId, const QString &portId)
{
    ProjectDocument candidate = m_projectService->document();
    for (RegisterGroup &group : candidate.groups)
    {
        if (group.id == groupId) { group.portId = portId; break; }
    }
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::moveGroup(const QString &groupId, int x, int y)
{
    ProjectDocument candidate = m_projectService->document();
    for (RegisterGroup &group : candidate.groups)
    {
        if (group.id == groupId)
        {
            group.canvasX = qMax(0, x);
            group.canvasY = qMax(0, y);
            break;
        }
    }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::importCsvIntoGroup(const QString &groupId,
                                                    const CsvImportResult &imported,
                                                    bool replaceGroup)
{
    if (!imported.result.success) { return imported.result; }
    ProjectDocument candidate = m_projectService->document();
    if (replaceGroup)
    {
        for (int i = candidate.registers.size() - 1; i >= 0; --i)
        {
            if (candidate.registers.at(i).groupId == groupId) { candidate.registers.removeAt(i); }
        }
    }
    for (RegisterPoint point : imported.registers)
    {
        point.groupId = groupId; // 忽略 CSV 分组列，全部归入本组
        candidate.registers.append(point);
    }
    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success) { return validation; }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult RegisterService::importCsvAsNewGroup(const RegisterGroup &group,
                                                     const CsvImportResult &imported)
{
    if (!imported.result.success)
    {
        return imported.result;
    }
    if (imported.registers.isEmpty())
    {
        return OperationResult::fail(QStringLiteral("csv_empty"),
                                     QStringLiteral("registers"),
                                     QStringLiteral("CSV 未解析到有效寄存器点位"));
    }

    RegisterGroup added = group;
    added.name = added.name.trimmed();
    if (added.name.isEmpty())
    {
        return OperationResult::fail(QStringLiteral("empty_group"),
                                     QStringLiteral("name"),
                                     QStringLiteral("分组名称不能为空"));
    }
    if (added.id.isEmpty())
    {
        added.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    added.isDefault = false;

    ProjectDocument candidate = m_projectService->document();
    for (const RegisterGroup &existing : candidate.groups)
    {
        if (existing.name == added.name)
        {
            return OperationResult::fail(QStringLiteral("duplicate_group"),
                                         QStringLiteral("name"),
                                         QStringLiteral("分组名称不允许重复：%1").arg(added.name));
        }
        if (existing.id == added.id)
        {
            return OperationResult::fail(QStringLiteral("duplicate_group_id"),
                                         QStringLiteral("id"),
                                         QStringLiteral("分组 ID 已存在"));
        }
    }

    if (added.canvasX == 0 && added.canvasY == 0)
    {
        const int index = candidate.groups.size();
        added.canvasX = 40 + (index % 3) * 280;
        added.canvasY = 40 + (index / 3) * 200;
    }

    candidate.groups.append(added);
    for (RegisterPoint point : imported.registers)
    {
        point.groupId = added.id;
        candidate.registers.append(point);
    }

    const OperationResult validation = ValidationService::validateProject(candidate);
    if (!validation.success)
    {
        return validation;
    }
    m_projectService->editableDocument() = candidate;
    m_projectService->markDirty();

    OperationResult result = OperationResult::ok();
    result.message = QStringLiteral("已导入分组「%1」，共 %2 条寄存器")
                         .arg(added.name)
                         .arg(imported.registers.size());
    return result;
}
