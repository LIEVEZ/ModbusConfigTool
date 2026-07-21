#include "group_realtime_panel.h"

#include "Domain/Models/domain_enums.h"
#include "Domain/Models/project_document.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

GroupRealtimePanel::GroupRealtimePanel(const QString &groupId,
                                       const ProjectDocument &doc,
                                       QWidget *parent)
    : QDialog(parent), m_groupId(groupId)
{
    setObjectName(QStringLiteral("groupRealtimePanel"));
    resize(720, 480);

    const RegisterGroup *group = nullptr;
    for (const RegisterGroup &candidate : doc.groups)
    {
        if (candidate.id == groupId)
        {
            group = &candidate;
            break;
        }
    }

    const QString groupName = group ? group->name : QStringLiteral("未知分组");
    const QString groupColor = group ? group->color : QStringLiteral("#f54e00");
    const QString groupDesc = group && !group->description.isEmpty()
        ? group->description
        : QStringLiteral("无描述");
    setWindowTitle(QStringLiteral("%1 · 实时数值").arg(groupName));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("realtimeHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 4);
    headerLayout->setSpacing(12);

    auto *swatch = new QWidget(header);
    swatch->setObjectName(QStringLiteral("realtimeSwatch"));
    swatch->setFixedSize(6, 34);
    swatch->setStyleSheet(QStringLiteral("background: %1; border-radius: 3px;").arg(groupColor));

    auto *titleBox = new QVBoxLayout;
    titleBox->setContentsMargins(0, 0, 0, 0);
    titleBox->setSpacing(2);
    auto *title = new QLabel(groupName, header);
    title->setObjectName(QStringLiteral("realtimeTitle"));
    auto *desc = new QLabel(groupDesc, header);
    desc->setObjectName(QStringLiteral("realtimeDescription"));
    desc->setWordWrap(true);
    titleBox->addWidget(title);
    titleBox->addWidget(desc);

    m_countBadge = new QLabel(header);
    m_countBadge->setObjectName(QStringLiteral("realtimeCountBadge"));
    m_countBadge->setAlignment(Qt::AlignCenter);

    headerLayout->addWidget(swatch, 0, Qt::AlignTop);
    headerLayout->addLayout(titleBox, 1);
    headerLayout->addWidget(m_countBadge, 0, Qt::AlignTop);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("groupRealtimeTable"));
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("从站"),
        QStringLiteral("地址"),
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("实时值")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);

    auto *buttons = new QDialogButtonBox(this);
    auto *configBtn = new QPushButton(QStringLiteral("寄存器配置"), this);
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setObjectName(QStringLiteral("primaryButton"));
    closeBtn->setDefault(true);
    buttons->addButton(configBtn, QDialogButtonBox::ActionRole);
    buttons->addButton(closeBtn, QDialogButtonBox::AcceptRole);

    layout->addWidget(header);
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);

    connect(configBtn, &QPushButton::clicked, this, [this]() {
        emit configureRegistersRequested(m_groupId);
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    updateValues(doc);
}

void GroupRealtimePanel::updateValues(const ProjectDocument &doc)
{
    m_table->setRowCount(0);
    int row = 0;
    for (const RegisterPoint &point : doc.registers)
    {
        if (point.groupId != m_groupId)
        {
            continue;
        }

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(point.slaveAddress)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(point.address)));
        m_table->setItem(row, 2, new QTableWidgetItem(point.name));
        m_table->setItem(row, 3, new QTableWidgetItem(dataTypeToString(point.dataType)));
        m_table->setItem(row, 4, new QTableWidgetItem(
            point.currentValue.toDisplayString(point.precision)));
        ++row;
    }

    if (m_countBadge)
    {
        m_countBadge->setText(QStringLiteral("%1 点").arg(row));
    }

    if (row == 0)
    {
        m_table->setRowCount(1);
        auto *empty = new QTableWidgetItem(QStringLiteral("暂无实时数据"));
        empty->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(0, 0, empty);
        m_table->setSpan(0, 0, 1, 5);
    }
}
