#include "group_realtime_panel.h"

#include "Domain/Models/project_document.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

GroupRealtimePanel::GroupRealtimePanel(const QString &groupId, const ProjectDocument &doc, QWidget *parent)
    : QDialog(parent), m_groupId(groupId)
{
    setWindowTitle(QStringLiteral("实时数值"));
    resize(700, 480);

    auto *layout = new QVBoxLayout(this);

    // Header
    const RegisterGroup *group = nullptr;
    for (const RegisterGroup &g : doc.groups)
    {
        if (g.id == groupId) { group = &g; break; }
    }

    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 12);
    auto *title = new QLabel(group ? group->name : QStringLiteral("未知分组"), header);
    QFont titleFont = title->font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    auto *desc = new QLabel(group && !group->description.isEmpty() ? group->description : QStringLiteral("无描述"), header);
    desc->setStyleSheet(QStringLiteral("color: rgba(38,37,30,0.6);"));
    headerLayout->addWidget(title);
    headerLayout->addWidget(desc);
    headerLayout->addStretch();

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({QStringLiteral("从站"), QStringLiteral("地址"), QStringLiteral("名称"), QStringLiteral("类型"), QStringLiteral("实时值")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setEditTriggers(QTableWidget::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    // Footer buttons
    auto *buttons = new QDialogButtonBox(this);
    auto *configBtn = new QPushButton(QStringLiteral("寄存器配置"), this);
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setDefault(true);
    buttons->addButton(configBtn, QDialogButtonBox::ActionRole);
    buttons->addButton(closeBtn, QDialogButtonBox::AcceptRole);

    layout->addWidget(header);
    layout->addWidget(m_table);
    layout->addWidget(buttons);

    connect(configBtn, &QPushButton::clicked, this, [this]() { emit configureRegistersRequested(m_groupId); });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    updateValues(doc);
}

void GroupRealtimePanel::updateValues(const ProjectDocument &doc)
{
    m_table->setRowCount(0);
    int row = 0;
    for (const RegisterPoint &point : doc.registers)
    {
        if (point.groupId != m_groupId) continue;

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(point.slaveAddress)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(point.address)));
        m_table->setItem(row, 2, new QTableWidgetItem(point.name));
        m_table->setItem(row, 3, new QTableWidgetItem(dataTypeToString(point.dataType)));
        m_table->setItem(row, 4, new QTableWidgetItem(point.currentValue.toDisplayString()));
        ++row;
    }
}
