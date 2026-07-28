#include "group_realtime_panel.h"

#include "Domain/Models/domain_enums.h"
#include "Domain/Models/project_factory.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
QTableWidgetItem *makeReadOnlyItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

bool isFloatingType(DataType type)
{
    return type == DataType::Float32 || type == DataType::Float64;
}

bool isSignedType(DataType type)
{
    return type == DataType::Int16 || type == DataType::Int32 || type == DataType::Int64;
}

QString strategyDisplayText(const StrategySpec &strategy)
{
    QString typeText;
    switch (strategy.type)
    {
    case StrategyType::Linear:
        typeText = QStringLiteral("线性");
        break;
    case StrategyType::Random:
        typeText = QStringLiteral("随机");
        break;
    case StrategyType::SineWave:
        typeText = QStringLiteral("正弦");
        break;
    case StrategyType::None:
    default:
        return QStringLiteral("无");
    }

    if (!strategy.enabled)
    {
        return typeText + QStringLiteral("·停用");
    }
    return typeText;
}
}

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

    auto *randomButton = new QToolButton(searchBar);
    randomButton->setObjectName(QStringLiteral("realtimeRandomButton"));
    randomButton->setText(QStringLiteral("随机值"));
    randomButton->setPopupMode(QToolButton::MenuButtonPopup);
    randomButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    randomButton->setFixedHeight(32);
    randomButton->setCursor(Qt::PointingHandCursor);
    randomButton->setToolTip(QStringLiteral("在点位 min/max 范围内生成随机实时值"));

    auto *randomMenu = new QMenu(randomButton);
    QAction *randomAllAction = randomMenu->addAction(QStringLiteral("随机全部点位"));
    QAction *randomFilteredAction = randomMenu->addAction(QStringLiteral("随机当前筛选结果"));
    randomButton->setMenu(randomMenu);
    randomButton->setDefaultAction(randomFilteredAction);

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(randomButton, 0);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("groupRealtimeTable"));
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("从站"),
        QStringLiteral("地址"),
        QStringLiteral("数量"),
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("编码"),
        QStringLiteral("协议键"),
        QStringLiteral("标签"),
        QStringLiteral("策略"),
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
    m_table->setColumnWidth(8, 80);
    m_table->setColumnWidth(9, 110);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setToolTip(QStringLiteral("双击“实时值”列可修改；将按该点协议键对应的数据类型校验"));

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
    connect(m_table, &QTableWidget::itemChanged, this, &GroupRealtimePanel::onItemChanged);
    connect(randomAllAction, &QAction::triggered, this, [this]() {
        // reuse slot via property-less direct call
        const QList<RegisterPoint *> targets = collectTargetPoints(false);
        if (targets.isEmpty())
        {
            QMessageBox::information(this, QStringLiteral("随机值"), QStringLiteral("当前分组没有可随机的点位。"));
            return;
        }
        QList<QPair<QString, RegisterValue>> writes;
        writes.reserve(targets.size());
        for (RegisterPoint *point : targets)
        {
            const RegisterValue value = randomValueInRange(*point);
            point->currentValue = value;
            writes.append(qMakePair(point->id, value));
        }
        emit bulkValuesWriteRequested(writes);
        rebuildTable();
    });
    connect(randomFilteredAction, &QAction::triggered, this, [this]() {
        const QList<RegisterPoint *> targets = collectTargetPoints(true);
        if (targets.isEmpty())
        {
            QMessageBox::information(this,
                                     QStringLiteral("随机值"),
                                     m_searchText.isEmpty()
                                         ? QStringLiteral("当前分组没有可随机的点位。")
                                         : QStringLiteral("当前筛选结果为空，请调整搜索条件。"));
            return;
        }
        QList<QPair<QString, RegisterValue>> writes;
        writes.reserve(targets.size());
        for (RegisterPoint *point : targets)
        {
            const RegisterValue value = randomValueInRange(*point);
            point->currentValue = value;
            writes.append(qMakePair(point->id, value));
        }
        emit bulkValuesWriteRequested(writes);
        rebuildTable();
    });
    // 主按钮默认执行“随机当前筛选结果”
    connect(randomButton, &QToolButton::clicked, randomFilteredAction, &QAction::trigger);

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

void GroupRealtimePanel::onRandomizeClicked()
{
    // kept for slot completeness; actions handle the work
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
        || contains(endianToString(point.endian))
        || contains(strategyDisplayText(point.strategy));
}

const RegisterPoint *GroupRealtimePanel::findPoint(const QString &pointId) const
{
    for (const RegisterPoint &point : m_document.registers)
    {
        if (point.id == pointId)
        {
            return &point;
        }
    }
    return nullptr;
}

QList<RegisterPoint *> GroupRealtimePanel::collectTargetPoints(bool filteredOnly)
{
    QList<RegisterPoint *> targets;
    for (RegisterPoint &point : m_document.registers)
    {
        if (point.groupId != m_groupId)
        {
            continue;
        }
        if (filteredOnly && !matchesSearch(point))
        {
            continue;
        }
        targets.append(&point);
    }
    return targets;
}

RegisterValue GroupRealtimePanel::randomValueInRange(const RegisterPoint &point)
{
    RegisterValue minimum = point.minimumValue;
    RegisterValue maximum = point.maximumValue;
    if (minimum.dataType() != point.dataType)
    {
        minimum = ProjectFactory::minimumFor(point.dataType);
    }
    if (maximum.dataType() != point.dataType)
    {
        maximum = ProjectFactory::maximumFor(point.dataType);
    }

    QRandomGenerator *rng = QRandomGenerator::global();

    if (isFloatingType(point.dataType))
    {
        double low = minimum.toDouble();
        double high = maximum.toDouble();
        if (high < low)
        {
            std::swap(low, high);
        }
        if (qFuzzyCompare(low, high))
        {
            return RegisterValue::fromFloating(low, point.dataType);
        }
        const double value = low + (high - low) * rng->generateDouble();
        return RegisterValue::fromFloating(value, point.dataType);
    }

    if (isSignedType(point.dataType))
    {
        qint64 low = minimum.toSigned64();
        qint64 high = maximum.toSigned64();
        if (high < low)
        {
            std::swap(low, high);
        }
        if (low == high)
        {
            return RegisterValue::fromSigned64(low, point.dataType);
        }
        // Use unsigned span to avoid overflow when high == qint64 max.
        const quint64 span = static_cast<quint64>(high) - static_cast<quint64>(low);
        const quint64 offset = rng->generate64() % (span + 1ULL);
        return RegisterValue::fromSigned64(low + static_cast<qint64>(offset), point.dataType);
    }

    // unsigned
    quint64 low = minimum.toUnsigned64();
    quint64 high = maximum.toUnsigned64();
    if (high < low)
    {
        std::swap(low, high);
    }
    if (low == high)
    {
        return RegisterValue::fromUnsigned64(low, point.dataType);
    }
    const quint64 span = high - low;
    const quint64 offset = rng->generate64() % (span + 1ULL);
    return RegisterValue::fromUnsigned64(low + offset, point.dataType);
}

void GroupRealtimePanel::rebuildTable()
{
    m_updatingTable = true;
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
        m_table->setItem(row, 0, makeReadOnlyItem(QString::number(point.slaveAddress)));
        m_table->setItem(row, 1, makeReadOnlyItem(QString::number(point.address)));
        m_table->setItem(row, 2, makeReadOnlyItem(QString::number(point.registerCount)));
        m_table->setItem(row, 3, makeReadOnlyItem(point.name));
        m_table->setItem(row, 4, makeReadOnlyItem(dataTypeToString(point.dataType)));
        m_table->setItem(row, 5, makeReadOnlyItem(endianToString(point.endian)));
        m_table->setItem(row, 6, makeReadOnlyItem(point.protocolKey));
        m_table->setItem(row, 7, makeReadOnlyItem(point.label));
        m_table->setItem(row, 8, makeReadOnlyItem(strategyDisplayText(point.strategy)));

        auto *valueItem = new QTableWidgetItem(point.currentValue.toDisplayString(point.precision));
        valueItem->setFlags(valueItem->flags() | Qt::ItemIsEditable);
        valueItem->setData(Qt::UserRole, point.id);
        valueItem->setData(Qt::UserRole + 1, point.protocolKey);
        valueItem->setData(Qt::UserRole + 2, point.currentValue.toDisplayString(point.precision));
        valueItem->setToolTip(QStringLiteral("协议键：%1\n类型：%2\n双击编辑后回车写入")
                                  .arg(point.protocolKey.isEmpty() ? QStringLiteral("（空）") : point.protocolKey)
                                  .arg(dataTypeToString(point.dataType)));
        m_table->setItem(row, kValueColumn, valueItem);
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
        auto *empty = makeReadOnlyItem(emptyText);
        empty->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(0, 0, empty);
        m_table->setSpan(0, 0, 1, 10);
    }

    m_updatingTable = false;
}

void GroupRealtimePanel::onItemChanged(QTableWidgetItem *item)
{
    if (m_updatingTable || !item || item->column() != kValueColumn)
    {
        return;
    }

    const QString pointId = item->data(Qt::UserRole).toString();
    const QString protocolKey = item->data(Qt::UserRole + 1).toString();
    const QString previousText = item->data(Qt::UserRole + 2).toString();
    const QString inputText = item->text().trimmed();

    const RegisterPoint *point = findPoint(pointId);
    if (!point)
    {
        m_updatingTable = true;
        item->setText(previousText);
        m_updatingTable = false;
        QMessageBox::warning(this,
                             QStringLiteral("写入失败"),
                             QStringLiteral("未找到对应寄存器点位，无法写入。"));
        return;
    }

    if (inputText == previousText.trimmed())
    {
        return;
    }

    const ValueResult parsed = RegisterValue::fromText(point->dataType, inputText);
    if (!parsed.result.success)
    {
        m_updatingTable = true;
        item->setText(previousText);
        m_updatingTable = false;

        const QString keyText = protocolKey.isEmpty()
            ? QStringLiteral("（空）")
            : protocolKey;
        QMessageBox::warning(
            this,
            QStringLiteral("数值无效"),
            QStringLiteral("协议键「%1」写入失败。\n\n"
                           "名称：%2\n"
                           "地址：%3\n"
                           "类型：%4\n"
                           "输入：%5\n"
                           "原因：%6")
                .arg(keyText,
                     point->name,
                     QString::number(point->address),
                     dataTypeToString(point->dataType),
                     inputText.isEmpty() ? QStringLiteral("（空）") : inputText,
                     parsed.result.detail.isEmpty() ? parsed.result.message : parsed.result.detail));
        return;
    }

    const QString display = parsed.value.toDisplayString(point->precision);
    m_updatingTable = true;
    item->setText(display);
    item->setData(Qt::UserRole + 2, display);
    m_updatingTable = false;

    for (RegisterPoint &localPoint : m_document.registers)
    {
        if (localPoint.id == pointId)
        {
            localPoint.currentValue = parsed.value;
            break;
        }
    }

    emit valueWriteRequested(pointId, parsed.value);
}