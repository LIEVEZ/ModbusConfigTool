#include "group_realtime_panel.h"

#include "Domain/Models/domain_enums.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

GroupRealtimePanel::GroupRealtimePanel(const QString &groupId,
                                       const ProjectDocument &doc,
                                       QWidget *parent)
    : QDialog(parent), m_groupId(groupId)
{
    setObjectName(QStringLiteral("groupRealtimePanel"));
    resize(980, 520);

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

    auto *searchBar = new QWidget(this);
    searchBar->setObjectName(QStringLiteral("realtimeSearchBar"));
    auto *searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(8);
    m_searchEdit = new QLineEdit(searchBar);
    m_searchEdit->setObjectName(QStringLiteral("realtimeSearchEdit"));
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索名称、地址、协议键或标签..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(32);
    searchLayout->addWidget(m_searchEdit, 1);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("groupRealtimeTable"));
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("从站"),
        QStringLiteral("地址"),
        QStringLiteral("数量"),
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("编码"),
        QStringLiteral("协议键"),
        QStringLiteral("标签"),
        QStringLiteral("实时值")
    });
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 55);
    m_table->setColumnWidth(1, 70);
    m_table->setColumnWidth(2, 50);
    m_table->setColumnWidth(4, 75);
    m_table->setColumnWidth(5, 65);
    m_table->setColumnWidth(7, 90);
    m_table->setColumnWidth(8, 90);
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
    layout->addWidget(searchBar);
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);

    connect(configBtn, &QPushButton::clicked, this, [this]() {
        emit configureRegistersRequested(m_groupId);
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &GroupRealtimePanel::onSearchTextChanged);

    updateValues(doc);
}

void GroupRealtimePanel::updateValues(const ProjectDocument &doc)
{
    m_document = doc;
    rebuildTable();
}

void GroupRealtimePanel::onSearchTextChanged(const QString &text)
{
    m_searchText = text.trimmed();
    rebuildTable();
}

bool GroupRealtimePanel::matchesSearch(const RegisterPoint &point) const
{
    if (m_searchText.isEmpty())
    {
        return true;
    }

    const QString needle = m_searchText;
    const auto contains = [&](const QString &value) {
        return value.contains(needle, Qt::CaseInsensitive);
    };

    return contains(point.name)
        || contains(QString::number(point.address))
        || contains(QString::number(point.slaveAddress))
        || contains(point.protocolKey)
        || contains(point.label)
        || contains(dataTypeToString(point.dataType))
        || contains(endianToString(point.endian));
}

void GroupRealtimePanel::rebuildTable()
{
    m_table->clearSpans();
    m_table->setRowCount(0);

    int total = 0;
    int visible = 0;
    for (const RegisterPoint &point : m_document.registers)
    {
        if (point.groupId != m_groupId)
        {
            continue;
        }
        ++total;
        if (!matchesSearch(point))
        {
            continue;
        }

        const int row = visible;
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(point.slaveAddress)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(point.address)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(point.registerCount)));
        m_table->setItem(row, 3, new QTableWidgetItem(point.name));
        m_table->setItem(row, 4, new QTableWidgetItem(dataTypeToString(point.dataType)));
        m_table->setItem(row, 5, new QTableWidgetItem(endianToString(point.endian)));
        m_table->setItem(row, 6, new QTableWidgetItem(point.protocolKey));
        m_table->setItem(row, 7, new QTableWidgetItem(point.label));
        m_table->setItem(row, 8, new QTableWidgetItem(
            point.currentValue.toDisplayString(point.precision)));
        ++visible;
    }

    if (m_countBadge)
    {
        if (!m_searchText.isEmpty() && visible != total)
        {
            m_countBadge->setText(QStringLiteral("%1 / %2 点").arg(visible).arg(total));
        }
        else
        {
            m_countBadge->setText(QStringLiteral("%1 点").arg(total));
        }
    }

    if (visible == 0)
    {
        m_table->setRowCount(1);
        const QString emptyText = total == 0
            ? QStringLiteral("暂无实时数据")
            : QStringLiteral("无匹配结果");
        auto *empty = new QTableWidgetItem(emptyText);
        empty->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(0, 0, empty);
        m_table->setSpan(0, 0, 1, 9);
    }
}
