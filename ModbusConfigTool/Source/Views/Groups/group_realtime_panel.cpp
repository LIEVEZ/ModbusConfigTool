#include "group_realtime_panel.h"

#include "Domain/Models/domain_enums.h"
#include "Domain/Models/project_factory.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QAbstractItemView>
#include <QToolTip>
#include <QClipboard>
#include <QCursor>
#include <QApplication>
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

QString valueCellToolTip(const RegisterPoint &point, const QString &display)
{
    return QStringLiteral("实时值：%1\n协议键：%2\n类型：%3\n双击编辑后回车写入")
        .arg(display.isEmpty() ? QStringLiteral("（空）") : display,
             point.protocolKey.isEmpty() ? QStringLiteral("（空）") : point.protocolKey,
             dataTypeToString(point.dataType));
}
}

GroupRealtimePanel::GroupRealtimePanel(const QString &groupId,
                                       const ProjectDocument &doc,
                                       QWidget *parent)
    : QDialog(parent), m_groupId(groupId)
{
    setObjectName(QStringLiteral("groupRealtimePanel"));
    resize(1180, 560);

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
    randomButton->setToolTip(QStringLiteral("在 ±10000 内生成随机实时值（浮点保留两位小数）"));

    auto *randomMenu = new QMenu(randomButton);
    QAction *randomAllAction = randomMenu->addAction(QStringLiteral("随机全部点位"));
    QAction *randomFilteredAction = randomMenu->addAction(QStringLiteral("随机当前筛选结果"));
    randomButton->setMenu(randomMenu);
    randomButton->setDefaultAction(randomFilteredAction);

    auto *resetButton = new QToolButton(searchBar);
    resetButton->setObjectName(QStringLiteral("realtimeResetButton"));
    resetButton->setText(QStringLiteral("重置值"));
    resetButton->setPopupMode(QToolButton::MenuButtonPopup);
    resetButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    resetButton->setFixedHeight(32);
    resetButton->setCursor(Qt::PointingHandCursor);
    resetButton->setToolTip(QStringLiteral("将实时值重置为 0（并限制在 min/max 范围内）"));

    auto *resetMenu = new QMenu(resetButton);
    QAction *resetAllAction = resetMenu->addAction(QStringLiteral("重置全部点位"));
    QAction *resetFilteredAction = resetMenu->addAction(QStringLiteral("重置当前筛选结果"));
    resetButton->setMenu(resetMenu);
    resetButton->setDefaultAction(resetFilteredAction);

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(randomButton, 0);
    searchLayout->addWidget(resetButton, 0);

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
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Interactive);
    m_table->setColumnWidth(0, 55);
    m_table->setColumnWidth(1, 70);
    m_table->setColumnWidth(2, 50);
    m_table->setColumnWidth(4, 75);
    m_table->setColumnWidth(5, 65);
    m_table->setColumnWidth(6, 130);
    m_table->setColumnWidth(7, 90);
    m_table->setColumnWidth(8, 80);
    m_table->setColumnWidth(9, 220);
    m_table->verticalHeader()->setVisible(false);
    m_table->setTextElideMode(Qt::ElideRight);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setToolTip(QStringLiteral("双击协议键可复制；双击其他列打开配置；双击实时值可改数值；悬停实时值可看完整数值"));

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
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &GroupRealtimePanel::onCellDoubleClicked);
    connect(randomAllAction, &QAction::triggered, this, [this]() {
        applyGeneratedValues(false, false);
    });
    connect(randomFilteredAction, &QAction::triggered, this, [this]() {
        applyGeneratedValues(true, false);
    });
    connect(randomButton, &QToolButton::clicked, randomFilteredAction, &QAction::trigger);

    connect(resetAllAction, &QAction::triggered, this, [this]() {
        applyGeneratedValues(false, true);
    });
    connect(resetFilteredAction, &QAction::triggered, this, [this]() {
        applyGeneratedValues(true, true);
    });
    connect(resetButton, &QToolButton::clicked, resetFilteredAction, &QAction::trigger);

    updateValues(doc);
}

void GroupRealtimePanel::updateValues(const ProjectDocument &doc)
{
    m_document = doc;
    if (tryUpdateValueCells())
    {
        return;
    }
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
        || contains(endianToString(point.endian))
        || contains(strategyDisplayText(point.strategy));
}

QString GroupRealtimePanel::pointIdAtRow(int row) const
{
    if (!m_table || row < 0 || row >= m_table->rowCount())
    {
        return QString();
    }

    if (QTableWidgetItem *valueItem = m_table->item(row, kValueColumn))
    {
        const QString pointId = valueItem->data(Qt::UserRole).toString();
        if (!pointId.isEmpty())
        {
            return pointId;
        }
    }

    if (QTableWidgetItem *firstItem = m_table->item(row, 0))
    {
        return firstItem->data(Qt::UserRole).toString();
    }
    return QString();
}

void GroupRealtimePanel::onCellDoubleClicked(int row, int column)
{
    if (column == kValueColumn)
    {
        return;
    }

    // 双击协议键：复制到剪贴板，方便联调粘贴
    if (column == kProtocolKeyColumn)
    {
        QTableWidgetItem *keyItem = m_table ? m_table->item(row, kProtocolKeyColumn) : nullptr;
        const QString protocolKey = keyItem ? keyItem->text().trimmed() : QString();
        if (protocolKey.isEmpty())
        {
            QToolTip::showText(QCursor::pos(), QStringLiteral("协议键为空，无法复制"), this);
            return;
        }

        if (QClipboard *clipboard = QApplication::clipboard())
        {
            clipboard->setText(protocolKey);
        }
        QToolTip::showText(QCursor::pos(),
                           QStringLiteral("已复制协议键：%1").arg(protocolKey),
                           this);
        return;
    }

    const QString pointId = pointIdAtRow(row);
    if (pointId.isEmpty())
    {
        return;
    }

    emit editRegisterRequested(pointId);
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

void GroupRealtimePanel::applyGeneratedValues(bool filteredOnly, bool resetToZero)
{
    const QList<RegisterPoint *> targets = collectTargetPoints(filteredOnly);
    const QString title = resetToZero ? QStringLiteral("重置值") : QStringLiteral("随机值");
    if (targets.isEmpty())
    {
        const QString emptyText = filteredOnly && !m_searchText.isEmpty()
            ? QStringLiteral("当前筛选结果为空，请调整搜索条件。")
            : QStringLiteral("当前分组没有可操作的点位。");
        QMessageBox::information(this, title, emptyText);
        return;
    }

    QList<QPair<QString, RegisterValue>> writes;
    writes.reserve(targets.size());
    for (RegisterPoint *point : targets)
    {
        const RegisterValue value = resetToZero
            ? resetValueInRange(*point)
            : randomValueInRange(*point);
        point->currentValue = value;
        writes.append(qMakePair(point->id, value));
    }

    emit bulkValuesWriteRequested(writes);
}

RegisterValue GroupRealtimePanel::resetValueInRange(const RegisterPoint &point)
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

    if (isFloatingType(point.dataType))
    {
        double low = minimum.toDouble();
        double high = maximum.toDouble();
        if (high < low)
        {
            std::swap(low, high);
        }
        double value = 0.0;
        if (value < low)
        {
            value = low;
        }
        else if (value > high)
        {
            value = high;
        }
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
        qint64 value = 0;
        if (value < low)
        {
            value = low;
        }
        else if (value > high)
        {
            value = high;
        }
        return RegisterValue::fromSigned64(value, point.dataType);
    }

    quint64 low = minimum.toUnsigned64();
    quint64 high = maximum.toUnsigned64();
    if (high < low)
    {
        std::swap(low, high);
    }
    quint64 value = 0ULL;
    if (value < low)
    {
        value = low;
    }
    else if (value > high)
    {
        value = high;
    }
    return RegisterValue::fromUnsigned64(value, point.dataType);
}

RegisterValue GroupRealtimePanel::randomValueInRange(const RegisterPoint &point)
{
    // 随机值统一限制在 ±10000，避免按数据类型全范围生成过大数值。
    constexpr qint64 kRandomAbsLimit = 10000;

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
        low = qMax(low, -static_cast<double>(kRandomAbsLimit));
        high = qMin(high, static_cast<double>(kRandomAbsLimit));
        if (high < low)
        {
            low = -static_cast<double>(kRandomAbsLimit);
            high = static_cast<double>(kRandomAbsLimit);
        }
        if (qFuzzyCompare(low, high))
        {
            const double fixed = std::round(low * 100.0) / 100.0;
            return RegisterValue::fromFloating(fixed, point.dataType);
        }
        const double raw = low + (high - low) * rng->generateDouble();
        // 浮点随机值固定保留两位小数。
        const double value = std::round(raw * 100.0) / 100.0;
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
        low = qMax(low, -kRandomAbsLimit);
        high = qMin(high, kRandomAbsLimit);
        if (high < low)
        {
            low = -kRandomAbsLimit;
            high = kRandomAbsLimit;
        }
        if (low == high)
        {
            return RegisterValue::fromSigned64(low, point.dataType);
        }
        const quint64 span = static_cast<quint64>(high - low);
        const quint64 offset = rng->generate64() % (span + 1ULL);
        return RegisterValue::fromSigned64(low + static_cast<qint64>(offset), point.dataType);
    }

    // unsigned：正数范围同样限制在 10000 以内
    quint64 low = minimum.toUnsigned64();
    quint64 high = maximum.toUnsigned64();
    if (high < low)
    {
        std::swap(low, high);
    }
    high = qMin(high, static_cast<quint64>(kRandomAbsLimit));
    if (low > high)
    {
        low = 0ULL;
        high = static_cast<quint64>(kRandomAbsLimit);
    }
    if (low == high)
    {
        return RegisterValue::fromUnsigned64(low, point.dataType);
    }
    const quint64 span = high - low;
    const quint64 offset = rng->generate64() % (span + 1ULL);
    return RegisterValue::fromUnsigned64(low + offset, point.dataType);
}

bool GroupRealtimePanel::tryUpdateValueCells()
{
    if (!m_table || m_updatingTable)
    {
        return false;
    }

    int expected = 0;
    for (const RegisterPoint &point : m_document.registers)
    {
        if (point.groupId != m_groupId)
        {
            continue;
        }
        if (!matchesSearch(point))
        {
            continue;
        }
        ++expected;
    }

    if (expected == 0 || m_table->rowCount() != expected)
    {
        return false;
    }

    // 仅就地刷新实时值/策略，避免整表重建卡顿
    m_updatingTable = true;
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QTableWidgetItem *valueItem = m_table->item(row, kValueColumn);
        if (!valueItem)
        {
            m_updatingTable = false;
            return false;
        }

        const QString pointId = valueItem->data(Qt::UserRole).toString();
        const RegisterPoint *point = findPoint(pointId);
        if (!point)
        {
            m_updatingTable = false;
            return false;
        }

        const QString display = point->currentValue.toDisplayString(point->precision);
        if (valueItem->text() != display)
        {
            valueItem->setText(display);
            valueItem->setData(Qt::UserRole + 2, display);
        }
        valueItem->setToolTip(valueCellToolTip(*point, display));

        if (QTableWidgetItem *strategyItem = m_table->item(row, 8))
        {
            const QString strategyText = strategyDisplayText(point->strategy);
            if (strategyItem->text() != strategyText)
            {
                strategyItem->setText(strategyText);
            }
        }
    }
    m_updatingTable = false;
    return true;
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
        auto setPointMeta = [&](QTableWidgetItem *item) {
            if (item)
            {
                item->setData(Qt::UserRole, point.id);
                item->setToolTip(QStringLiteral("双击打开该点配置"));
            }
        };

        auto *slaveItem = makeReadOnlyItem(QString::number(point.slaveAddress));
        auto *addressItem = makeReadOnlyItem(QString::number(point.address));
        auto *countItem = makeReadOnlyItem(QString::number(point.registerCount));
        auto *nameItem = makeReadOnlyItem(point.name);
        auto *typeItem = makeReadOnlyItem(dataTypeToString(point.dataType));
        auto *endianItem = makeReadOnlyItem(endianToString(point.endian));
        auto *keyItem = makeReadOnlyItem(point.protocolKey);
        auto *labelItem = makeReadOnlyItem(point.label);
        auto *strategyItem = makeReadOnlyItem(strategyDisplayText(point.strategy));
        setPointMeta(slaveItem);
        setPointMeta(addressItem);
        setPointMeta(countItem);
        setPointMeta(nameItem);
        setPointMeta(typeItem);
        setPointMeta(endianItem);
        setPointMeta(keyItem);
        if (keyItem)
        {
            keyItem->setToolTip(point.protocolKey.isEmpty()
                                    ? QStringLiteral("协议键为空")
                                    : QStringLiteral("双击复制协议键：%1").arg(point.protocolKey));
        }
        setPointMeta(labelItem);
        setPointMeta(strategyItem);
        m_table->setItem(row, 0, slaveItem);
        m_table->setItem(row, 1, addressItem);
        m_table->setItem(row, 2, countItem);
        m_table->setItem(row, 3, nameItem);
        m_table->setItem(row, 4, typeItem);
        m_table->setItem(row, 5, endianItem);
        m_table->setItem(row, 6, keyItem);
        m_table->setItem(row, 7, labelItem);
        m_table->setItem(row, 8, strategyItem);

        const QString display = point.currentValue.toDisplayString(point.precision);
        auto *valueItem = new QTableWidgetItem(display);
        valueItem->setFlags(valueItem->flags() | Qt::ItemIsEditable);
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valueItem->setData(Qt::UserRole, point.id);
        valueItem->setData(Qt::UserRole + 1, point.protocolKey);
        valueItem->setData(Qt::UserRole + 2, display);
        valueItem->setToolTip(valueCellToolTip(point, display));
        m_table->setItem(row, kValueColumn, valueItem);
        ++visible;
    }

    m_table->resizeColumnToContents(kValueColumn);
    const int valueWidth = m_table->columnWidth(kValueColumn);
    m_table->setColumnWidth(kValueColumn, qBound(180, valueWidth + 12, 360));

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
    item->setToolTip(valueCellToolTip(*point, display));
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