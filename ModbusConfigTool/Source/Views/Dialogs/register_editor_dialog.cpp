#include "register_editor_dialog.h"

#include "Domain/Models/project_factory.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

RegisterEditorDialog::RegisterEditorDialog(const RegisterPoint &point,
                                           const QList<RegisterGroup> &groups,
                                           QWidget *parent)
    : QDialog(parent), m_original(point)
{
    setWindowTitle(QStringLiteral("寄存器详情")); resize(520, 520);
    auto *layout = new QVBoxLayout(this); auto *form = new QFormLayout;
    m_group = new QComboBox(this); for (const RegisterGroup &group : groups) { m_group->addItem(group.name, group.id); }
    m_slave = new QSpinBox(this); m_slave->setRange(1, 247);
    m_address = new QSpinBox(this); m_address->setRange(0, 65535);
    m_name = new QLineEdit(this); m_dataType = new QComboBox(this);
    m_dataType->addItems({QStringLiteral("INT16"), QStringLiteral("UINT16"), QStringLiteral("INT32"), QStringLiteral("UINT32"), QStringLiteral("FLOAT32"), QStringLiteral("INT64"), QStringLiteral("UINT64"), QStringLiteral("FLOAT64")});
    m_endian = new QComboBox(this); m_endian->addItems({QStringLiteral("BIG"), QStringLiteral("LITTLE"), QStringLiteral("LITSWAP")});
    m_storage = new QComboBox(this); m_storage->addItems({QStringLiteral("holding"), QStringLiteral("input")});
    m_protocolKey = new QLineEdit(this); m_value = new QLineEdit(this); m_enabled = new QCheckBox(QStringLiteral("启用"), this);
    m_strategyType = new QComboBox(this); m_strategyType->addItems({QStringLiteral("none"), QStringLiteral("linear"), QStringLiteral("random"), QStringLiteral("sine_wave")});
    m_strategyEnabled = new QCheckBox(QStringLiteral("启用模拟策略"), this);
    form->addRow(QStringLiteral("分组"), m_group); form->addRow(QStringLiteral("从站地址"), m_slave);
    form->addRow(QStringLiteral("寄存器地址"), m_address); form->addRow(QStringLiteral("名称"), m_name);
    form->addRow(QStringLiteral("数据类型"), m_dataType); form->addRow(QStringLiteral("字节序"), m_endian);
    form->addRow(QStringLiteral("存储区"), m_storage); form->addRow(QStringLiteral("协议键"), m_protocolKey);
    form->addRow(QStringLiteral("当前值"), m_value); form->addRow(QStringLiteral("策略类型"), m_strategyType);
    form->addRow(QString(), m_strategyEnabled); form->addRow(QString(), m_enabled);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addLayout(form); layout->addWidget(buttons);
    m_group->setCurrentIndex(m_group->findData(point.groupId)); m_slave->setValue(point.slaveAddress);
    m_address->setValue(point.address); m_name->setText(point.name); m_dataType->setCurrentText(dataTypeToString(point.dataType));
    m_endian->setCurrentText(endianToString(point.endian)); m_storage->setCurrentText(storageTypeToString(point.storageType));
    m_protocolKey->setText(point.protocolKey); m_value->setText(point.currentValue.toStorageString()); m_enabled->setChecked(point.enabled);
    m_strategyType->setCurrentText(strategyTypeToString(point.strategy.type)); m_strategyEnabled->setChecked(point.strategy.enabled);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

RegisterPoint RegisterEditorDialog::point() const
{
    RegisterPoint output = m_original;
    output.groupId = m_group->currentData().toString(); output.slaveAddress = quint8(m_slave->value());
    output.address = quint16(m_address->value()); output.name = m_name->text().trimmed();
    dataTypeFromString(m_dataType->currentText(), &output.dataType); endianFromString(m_endian->currentText(), &output.endian);
    storageTypeFromString(m_storage->currentText(), &output.storageType);
    output.registerCount = ProjectFactory::registerCountFor(output.dataType);
    output.protocolKey = m_protocolKey->text().trimmed(); output.enabled = m_enabled->isChecked();
    const ValueResult value = RegisterValue::fromText(output.dataType, m_value->text());
    if (value.result.success) { output.currentValue = value.value; }
    output.minimumValue = ProjectFactory::minimumFor(output.dataType); output.maximumValue = ProjectFactory::maximumFor(output.dataType);
    output.readFunctionCode = output.storageType == StorageType::Holding ? 3 : 4;
    output.writeFunctionCode = output.storageType == StorageType::Holding ? (output.registerCount == 1 ? 6 : 16) : 0;
    strategyTypeFromString(m_strategyType->currentText(), &output.strategy.type);
    output.strategy.enabled = m_strategyEnabled->isChecked() && output.strategy.type != StrategyType::None;
    if (output.strategy.type == StrategyType::Linear && output.strategy.parameters.isEmpty())
    {
        output.strategy.parameters = {{QStringLiteral("startValue"), 0.0}, {QStringLiteral("endValue"), 100.0}, {QStringLiteral("step"), 1.0}, {QStringLiteral("intervalMs"), 1000}};
    }
    else if (output.strategy.type == StrategyType::Random && output.strategy.parameters.isEmpty())
    {
        output.strategy.parameters = {{QStringLiteral("minValue"), 0.0}, {QStringLiteral("maxValue"), 100.0}, {QStringLiteral("intervalMs"), 1000}};
    }
    else if (output.strategy.type == StrategyType::SineWave && output.strategy.parameters.isEmpty())
    {
        output.strategy.parameters = {{QStringLiteral("amplitude"), 1.0}, {QStringLiteral("frequencyHz"), 1.0}, {QStringLiteral("center"), 0.0}, {QStringLiteral("intervalMs"), 100}};
    }
    return output;
}
