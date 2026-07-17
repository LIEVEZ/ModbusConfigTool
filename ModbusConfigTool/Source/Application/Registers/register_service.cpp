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
        if (candidate.groups.at(index).isDefault) { defaultId = candidate.groups.at(index).id; }
        if (candidate.groups.at(index).id == groupId) { groupIndex = index; }
    }
    if (groupIndex < 0) { return OperationResult::fail(QStringLiteral("missing_group"), QStringLiteral("groupId"), QStringLiteral("分组不存在")); }
    if (candidate.groups.at(groupIndex).isDefault) { return OperationResult::fail(QStringLiteral("default_group"), QStringLiteral("groupId"), QStringLiteral("默认分组不能删除")); }
    for (int index = candidate.registers.size() - 1; index >= 0; --index)
    {
        if (candidate.registers.at(index).groupId != groupId) { continue; }
        if (removePoints) { candidate.registers.removeAt(index); }
        else { candidate.registers[index].groupId = defaultId; }
    }
    candidate.groups.removeAt(groupIndex);
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
