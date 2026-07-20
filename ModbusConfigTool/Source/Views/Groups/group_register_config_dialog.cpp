#include "group_register_config_dialog.h"

#include "Domain/Models/project_document.h"
#include "Views/Registers/register_table_model.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

GroupRegisterConfigDialog::GroupRegisterConfigDialog(const QString &groupId, const ProjectDocument &doc, QWidget *parent)
    : QDialog(parent), m_groupId(groupId)
{
    setWindowTitle(QStringLiteral("寄存器配置"));
    resize(960, 600);

    auto *layout = new QVBoxLayout(this);

    // Header
    const RegisterGroup *group = nullptr;
    for (const RegisterGroup &g : doc.groups)
    {
        if (g.id == groupId) { group = &g; break; }
    }

    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 8);
    auto *title = new QLabel(group ? group->name : QStringLiteral("未知分组"), header);
    QFont titleFont = title->font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    // Toolbar
    auto *toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *addAction = toolbar->addAction(QStringLiteral("＋ 新增寄存器"));
    auto *editAction = toolbar->addAction(QStringLiteral("✎ 编辑"));
    auto *removeAction = toolbar->addAction(QStringLiteral("✕ 删除"));
    toolbar->addSeparator();
    auto *importAction = toolbar->addAction(QStringLiteral("↓ 导入 CSV"));
    auto *exportAction = toolbar->addAction(QStringLiteral("↑ 导出 CSV"));

    // Table
    m_table = new QTableView(this);
    m_table->setObjectName(QStringLiteral("groupRegisterTable"));
    m_model = new RegisterTableModel(this);
    setDocument(doc);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QTableView::SelectRows);
    m_table->setSelectionMode(QTableView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);

    // Footer
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);

    layout->addWidget(header);
    layout->addWidget(toolbar);
    layout->addWidget(m_table);
    layout->addWidget(buttons);

    connect(addAction, &QAction::triggered, this, &GroupRegisterConfigDialog::onAddRegister);
    connect(editAction, &QAction::triggered, this, &GroupRegisterConfigDialog::onEditRegister);
    connect(removeAction, &QAction::triggered, this, &GroupRegisterConfigDialog::onRemoveRegisters);
    connect(importAction, &QAction::triggered, this, &GroupRegisterConfigDialog::onImportCsv);
    connect(exportAction, &QAction::triggered, this, &GroupRegisterConfigDialog::onExportCsv);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
}

void GroupRegisterConfigDialog::setDocument(const ProjectDocument &doc)
{
    m_filteredDoc = doc;
    m_filteredDoc.registers.erase(
        std::remove_if(m_filteredDoc.registers.begin(),
                       m_filteredDoc.registers.end(),
                       [this](const RegisterPoint &point)
        {
            return point.groupId != m_groupId;
        }),
        m_filteredDoc.registers.end());
    if (m_model)
    {
        m_model->setDocument(&m_filteredDoc);
    }
}

void GroupRegisterConfigDialog::onAddRegister()
{
    emit addRegisterRequested(m_groupId);
}

void GroupRegisterConfigDialog::onEditRegister()
{
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    const int row = selected.first().row();
    const QString pointId = m_model->pointId(row);
    if (!pointId.isEmpty())
    {
        emit editRegisterRequested(pointId);
    }
}

void GroupRegisterConfigDialog::onRemoveRegisters()
{
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QStringList ids;
    for (const QModelIndex &index : selected)
    {
        const QString pointId = m_model->pointId(index.row());
        if (!pointId.isEmpty())
        {
            ids.append(pointId);
        }
    }
    emit removeRegistersRequested(ids);
}

void GroupRegisterConfigDialog::onImportCsv()
{
    emit importCsvRequested(m_groupId);
}

void GroupRegisterConfigDialog::onExportCsv()
{
    emit exportCsvRequested(m_groupId);
}
