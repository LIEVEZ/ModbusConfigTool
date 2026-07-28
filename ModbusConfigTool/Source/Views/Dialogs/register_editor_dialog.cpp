#include "register_editor_dialog.h"

#include "Domain/Models/project_factory.h"
#include "Views/Dialogs/strategy_editor_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
QString functionCodeText(int code)
{
    if (code <= 0)
    {
        return QStringLiteral("—");
    }
    return QStringLiteral("0x%1").arg(code, 2, 16, QLatin1Char('0')).toUpper();
}

void applyReadonlyStyle(QLabel *label)
{
    if (!label)
    {
        return;
    }
    label->setObjectName(QStringLiteral("registerEditorReadonlyValue"));
    label->setMinimumHeight(34);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
}
}

RegisterEditorDialog::RegisterEditorDialog(const RegisterPoint &point,
                                           const QList<RegisterGroup> &groups,
                                           QWidget *parent)
    : QDialog(parent), m_original(point)
{
    setObjectName(QStringLiteral("registerEditorDialog"));
    setWindowTitle(QStringLiteral("寄存器详情"));
    resize(620, 760);
    setMinimumWidth(560);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 16, 18, 16);
    root->setSpacing(14);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("registerEditorHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);
    headerLayout->setSpacing(12);

    auto *titleBlock = new QVBoxLayout;
    titleBlock->setContentsMargins(0, 0, 0, 0);
    titleBlock->setSpacing(4);
    auto *titleLabel = new QLabel(QStringLiteral("寄存器详情"), header);
    titleLabel->setObjectName(QStringLiteral("registerEditorTitle"));
    m_subtitleLabel = new QLabel(header);
    m_subtitleLabel->setObjectName(QStringLiteral("registerEditorSubtitle"));
    m_subtitleLabel->setWordWrap(true);
    titleBlock->addWidget(titleLabel);
    titleBlock->addWidget(m_subtitleLabel);

    headerLayout->addLayout(titleBlock, 1);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("registerEditorScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("registerEditorContent"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(2, 2, 10, 2);
    contentLayout->setSpacing(12);

    // 基础信息
    auto *basicBody = new QWidget(content);
    auto *basicGrid = new QGridLayout(basicBody);
    basicGrid->setContentsMargins(0, 0, 0, 0);
    basicGrid->setHorizontalSpacing(12);
    basicGrid->setVerticalSpacing(12);

    m_group = new QComboBox(basicBody);
    for (const RegisterGroup &group : groups)
    {
        m_group->addItem(group.name, group.id);
    }
    m_name = new QLineEdit(basicBody);
    m_name->setPlaceholderText(QStringLiteral("例如：pcs实际总有功功率(汇总)"));
    m_protocolKey = new QLineEdit(basicBody);
    m_protocolKey->setPlaceholderText(QStringLiteral("例如：pcsActivePowerSum"));
    m_category = new QLineEdit(basicBody);
    m_category->setPlaceholderText(QStringLiteral("可选，如 energy / status"));
    m_label = new QLineEdit(basicBody);
    m_label->setPlaceholderText(QStringLiteral("可选，便于筛选和标注"));

    basicGrid->addWidget(createField(QStringLiteral("分组"), m_group), 0, 0);
    basicGrid->addWidget(createField(QStringLiteral("名称"), m_name), 0, 1);
    basicGrid->addWidget(createField(QStringLiteral("协议键"), m_protocolKey), 1, 0, 1, 2);
    basicGrid->addWidget(createField(QStringLiteral("分类"), m_category), 2, 0);
    basicGrid->addWidget(createField(QStringLiteral("标签"), m_label), 2, 1);
    contentLayout->addWidget(createSectionCard(QStringLiteral("基础信息"),
                                               QStringLiteral("定义寄存器身份与业务语义"),
                                               basicBody));

    // 通信参数
    auto *commBody = new QWidget(content);
    auto *commGrid = new QGridLayout(commBody);
    commGrid->setContentsMargins(0, 0, 0, 0);
    commGrid->setHorizontalSpacing(12);
    commGrid->setVerticalSpacing(12);

    m_slave = new QSpinBox(commBody);
    m_slave->setRange(1, 247);
    m_address = new QSpinBox(commBody);
    m_address->setRange(0, 65535);
    m_storage = new QComboBox(commBody);
    m_storage->addItems({QStringLiteral("holding"), QStringLiteral("input")});
    m_dataType = new QComboBox(commBody);
    m_dataType->addItems({
        QStringLiteral("INT16"), QStringLiteral("UINT16"), QStringLiteral("INT32"),
        QStringLiteral("UINT32"), QStringLiteral("FLOAT32"), QStringLiteral("FLOAT"),
        QStringLiteral("INT64"), QStringLiteral("UINT64"), QStringLiteral("FLOAT64"),
        QStringLiteral("DFLOAT")
    });
    m_endian = new QComboBox(commBody);
    m_endian->addItems({
        QStringLiteral("BIG"), QStringLiteral("LITTLE"), QStringLiteral("LITSWAP"),
        QStringLiteral("BIGSWAP"), QStringLiteral("BIGBCD"), QStringLiteral("LITBCD")
    });
    m_registerCount = new QLabel(commBody);
    m_readCode = new QLabel(commBody);
    m_writeCode = new QLabel(commBody);
    applyReadonlyStyle(m_registerCount);
    applyReadonlyStyle(m_readCode);
    applyReadonlyStyle(m_writeCode);

    commGrid->addWidget(createField(QStringLiteral("从站地址"), m_slave), 0, 0);
    commGrid->addWidget(createField(QStringLiteral("寄存器地址"), m_address), 0, 1);
    commGrid->addWidget(createField(QStringLiteral("存储区"), m_storage), 1, 0);
    commGrid->addWidget(createField(QStringLiteral("数据类型"), m_dataType), 1, 1);
    commGrid->addWidget(createField(QStringLiteral("字节序"), m_endian), 2, 0);
    commGrid->addWidget(createField(QStringLiteral("寄存器数量"), m_registerCount), 2, 1);
    commGrid->addWidget(createField(QStringLiteral("读功能码"), m_readCode), 3, 0);
    commGrid->addWidget(createField(QStringLiteral("写功能码"), m_writeCode), 3, 1);
    contentLayout->addWidget(createSectionCard(QStringLiteral("通信参数"),
                                               QStringLiteral("数量与功能码会根据类型/存储区自动推导"),
                                               commBody));

    // 工程换算
    auto *scaleBody = new QWidget(content);
    auto *scaleGrid = new QGridLayout(scaleBody);
    scaleGrid->setContentsMargins(0, 0, 0, 0);
    scaleGrid->setHorizontalSpacing(12);
    scaleGrid->setVerticalSpacing(12);

    m_offset = new QDoubleSpinBox(scaleBody);
    m_offset->setDecimals(6);
    m_offset->setRange(-1e12, 1e12);
    m_precision = new QSpinBox(scaleBody);
    m_precision->setRange(0, 12);
    m_unit = new QLineEdit(scaleBody);
    m_unit->setPlaceholderText(QStringLiteral("例如 kW / A / V / %"));

    scaleGrid->addWidget(createField(QStringLiteral("偏移"), m_offset), 0, 0);
    scaleGrid->addWidget(createField(QStringLiteral("精度"), m_precision), 0, 1);
    scaleGrid->addWidget(createField(QStringLiteral("单位"), m_unit), 1, 0, 1, 2);
    contentLayout->addWidget(createSectionCard(QStringLiteral("工程换算"),
                                               QStringLiteral("用于显示与业务侧解释，不影响原始寄存器编码"),
                                               scaleBody));

    // 实时值
    auto *valueBody = new QWidget(content);
    auto *valueLayout = new QVBoxLayout(valueBody);
    valueLayout->setContentsMargins(0, 0, 0, 0);
    valueLayout->setSpacing(10);

    auto *valueRow = new QWidget(valueBody);
    auto *valueRowLayout = new QHBoxLayout(valueRow);
    valueRowLayout->setContentsMargins(0, 0, 0, 0);
    valueRowLayout->setSpacing(10);

    m_value = new QLineEdit(valueRow);
    m_value->setPlaceholderText(QStringLiteral("输入当前值"));
    m_unitSuffix = new QLabel(valueRow);
    m_unitSuffix->setObjectName(QStringLiteral("registerEditorUnitSuffix"));
    m_manualWriteButton = new QPushButton(QStringLiteral("写入当前值"), valueRow);
    m_manualWriteButton->setObjectName(QStringLiteral("primaryButton"));
    m_manualWriteButton->setCursor(Qt::PointingHandCursor);

    valueRowLayout->addWidget(m_value, 1);
    valueRowLayout->addWidget(m_unitSuffix);
    valueRowLayout->addWidget(m_manualWriteButton);

    auto *valueHint = new QLabel(QStringLiteral("holding 支持手动写入；input 只读显示。"), valueBody);
    valueHint->setObjectName(QStringLiteral("registerEditorHint"));
    valueHint->setWordWrap(true);

    valueLayout->addWidget(createField(QStringLiteral("当前值"), valueRow));
    valueLayout->addWidget(valueHint);
    contentLayout->addWidget(createSectionCard(QStringLiteral("实时值"),
                                               QStringLiteral("查看与手动下发当前寄存器值"),
                                               valueBody));

    // 模拟策略
    auto *strategyBody = new QWidget(content);
    auto *strategyLayout = new QVBoxLayout(strategyBody);
    strategyLayout->setContentsMargins(0, 0, 0, 0);
    strategyLayout->setSpacing(0);
    m_strategyEditor = new StrategyEditorWidget(strategyBody);
    strategyLayout->addWidget(m_strategyEditor);
    contentLayout->addWidget(createSectionCard(QStringLiteral("模拟策略"),
                                               QStringLiteral("配置运行时自动变化规则"),
                                               strategyBody));
    contentLayout->addStretch(1);
    scroll->setWidget(content);

    auto *footer = new QWidget(this);
    footer->setObjectName(QStringLiteral("registerEditorFooter"));
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(4, 4, 4, 0);
    footerLayout->setSpacing(10);
    footerLayout->addStretch(1);

    auto *buttons = new QDialogButtonBox(footer);
    auto *cancelButton = buttons->addButton(QStringLiteral("取消"), QDialogButtonBox::RejectRole);
    auto *saveButton = buttons->addButton(QStringLiteral("保存"), QDialogButtonBox::AcceptRole);
    saveButton->setObjectName(QStringLiteral("primaryButton"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    saveButton->setCursor(Qt::PointingHandCursor);
    footerLayout->addWidget(buttons);

    root->addWidget(header);
    root->addWidget(scroll, 1);
    root->addWidget(footer);

    m_group->setCurrentIndex(m_group->findData(point.groupId));
    m_slave->setValue(point.slaveAddress);
    m_address->setValue(point.address);
    m_name->setText(point.name);
    m_dataType->setCurrentText(dataTypeToString(point.dataType));
    m_endian->setCurrentText(endianToString(point.endian));
    m_storage->setCurrentText(storageTypeToString(point.storageType));
    m_protocolKey->setText(point.protocolKey);
    m_category->setText(point.category);
    m_label->setText(point.label);
    m_offset->setValue(point.offset);
    m_precision->setValue(point.precision);
    m_unit->setText(point.unit);
    m_value->setText(point.currentValue.toStorageString());
    m_strategyEditor->setStrategy(point.strategy);

    refreshDerivedFields();
    refreshSubtitle();
    refreshWriteAvailability();

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_manualWriteButton, &QPushButton::clicked, this, [this]()
    {
        DataType type = DataType::UInt16;
        dataTypeFromString(m_dataType->currentText(), &type);
        const ValueResult value = RegisterValue::fromText(type, m_value->text());
        if (value.result.success)
        {
            emit manualWriteRequested(value.value);
        }
    });
    connect(m_dataType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { refreshDerivedFields(); });
    connect(m_storage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
    {
        refreshDerivedFields();
        refreshWriteAvailability();
    });
    connect(m_slave, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { refreshSubtitle(); });
    connect(m_address, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { refreshSubtitle(); });
    connect(m_protocolKey, &QLineEdit::textChanged, this, [this](const QString &)
    {
        refreshSubtitle();
    });
    connect(m_name, &QLineEdit::textChanged, this, [this](const QString &)
    {
        refreshSubtitle();
    });
    connect(m_unit, &QLineEdit::textChanged, this, [this](const QString &)
    {
        refreshDerivedFields();
    });
}

RegisterPoint RegisterEditorDialog::point() const
{
    RegisterPoint output = m_original;
    output.groupId = m_group->currentData().toString();
    output.slaveAddress = quint8(m_slave->value());
    output.address = quint16(m_address->value());
    output.name = m_name->text().trimmed();
    dataTypeFromString(m_dataType->currentText(), &output.dataType);
    endianFromString(m_endian->currentText(), &output.endian);
    storageTypeFromString(m_storage->currentText(), &output.storageType);
    output.registerCount = ProjectFactory::registerCountFor(output.dataType);
    output.protocolKey = m_protocolKey->text().trimmed();
    output.category = m_category->text().trimmed();
    output.label = m_label->text().trimmed();
    output.offset = m_offset->value();
    output.precision = m_precision->value();
    output.unit = m_unit->text().trimmed();
    output.enabled = true;

    const ValueResult value = RegisterValue::fromText(output.dataType, m_value->text());
    if (value.result.success)
    {
        output.currentValue = value.value;
    }

    output.minimumValue = ProjectFactory::minimumFor(output.dataType);
    output.maximumValue = ProjectFactory::maximumFor(output.dataType);
    output.readFunctionCode = output.storageType == StorageType::Holding ? 3 : 4;
    output.writeFunctionCode = output.storageType == StorageType::Holding
        ? (output.registerCount == 1 ? 6 : 16)
        : 0;
    output.strategy = m_strategyEditor->strategy();
    return output;
}

QWidget *RegisterEditorDialog::createSectionCard(const QString &title,
                                                 const QString &subtitle,
                                                 QWidget *body) const
{
    auto *card = new QWidget;
    card->setObjectName(QStringLiteral("registerEditorCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(12);

    auto *header = new QVBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(2);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("registerEditorCardTitle"));
    auto *subtitleLabel = new QLabel(subtitle, card);
    subtitleLabel->setObjectName(QStringLiteral("registerEditorCardSubtitle"));
    subtitleLabel->setWordWrap(true);
    header->addWidget(titleLabel);
    header->addWidget(subtitleLabel);

    body->setParent(card);
    layout->addLayout(header);
    layout->addWidget(body);
    return card;
}

QLabel *RegisterEditorDialog::createFieldLabel(const QString &text) const
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("registerEditorFieldLabel"));
    return label;
}

QWidget *RegisterEditorDialog::createField(const QString &label, QWidget *editor) const
{
    auto *field = new QWidget;
    field->setObjectName(QStringLiteral("registerEditorField"));
    auto *layout = new QVBoxLayout(field);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(createFieldLabel(label));
    editor->setParent(field);
    layout->addWidget(editor);
    return field;
}

void RegisterEditorDialog::refreshDerivedFields()
{
    DataType type = DataType::UInt16;
    dataTypeFromString(m_dataType->currentText(), &type);
    StorageType storage = StorageType::Holding;
    storageTypeFromString(m_storage->currentText(), &storage);

    const quint16 count = ProjectFactory::registerCountFor(type);
    const int readCode = storage == StorageType::Holding ? 3 : 4;
    const int writeCode = storage == StorageType::Holding ? (count == 1 ? 6 : 16) : 0;

    m_registerCount->setText(QString::number(count));
    m_readCode->setText(functionCodeText(readCode));
    m_writeCode->setText(functionCodeText(writeCode));

    const QString unit = m_unit->text().trimmed();
    m_unitSuffix->setText(unit);
    m_unitSuffix->setVisible(!unit.isEmpty());
}

void RegisterEditorDialog::refreshSubtitle()
{
    const QString name = m_name->text().trimmed();
    const QString key = m_protocolKey->text().trimmed();
    const QString identity = !key.isEmpty() ? key : (!name.isEmpty() ? name : QStringLiteral("未命名寄存器"));
    m_subtitleLabel->setText(QStringLiteral("从站 %1  ·  地址 %2  ·  %3")
                                 .arg(m_slave->value())
                                 .arg(m_address->value())
                                 .arg(identity));
}

void RegisterEditorDialog::refreshWriteAvailability()
{
    StorageType storage = StorageType::Holding;
    storageTypeFromString(m_storage->currentText(), &storage);
    const bool writable = storage == StorageType::Holding;
    m_manualWriteButton->setEnabled(writable);
    m_manualWriteButton->setToolTip(writable
        ? QStringLiteral("将当前值写入设备")
        : QStringLiteral("input 存储区不支持写入"));
}
