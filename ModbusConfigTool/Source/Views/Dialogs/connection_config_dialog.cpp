#include "connection_config_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

ConnectionConfigDialog::ConnectionConfigDialog(const ServerProfile &profile, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("连接配置"));
    resize(480, 320);
    auto *layout = new QVBoxLayout(this);
    auto *common = new QFormLayout;
    m_type = new QComboBox(this); m_type->addItems({QStringLiteral("TCP Server"), QStringLiteral("RTU Slave")});
    m_pollInterval = new QSpinBox(this); m_pollInterval->setRange(100, 60000); m_pollInterval->setSuffix(QStringLiteral(" ms"));
    common->addRow(QStringLiteral("连接类型"), m_type); common->addRow(QStringLiteral("轮询周期"), m_pollInterval);
    m_pages = new QStackedWidget(this);
    auto *tcpPage = new QWidget(this); auto *tcpForm = new QFormLayout(tcpPage);
    m_host = new QLineEdit(tcpPage); m_port = new QSpinBox(tcpPage); m_port->setRange(1, 65535);
    tcpForm->addRow(QStringLiteral("主机地址"), m_host); tcpForm->addRow(QStringLiteral("端口"), m_port);
    auto *rtuPage = new QWidget(this); auto *rtuForm = new QFormLayout(rtuPage);
    m_serialPort = new QComboBox(rtuPage);
    for (const QSerialPortInfo &port : QSerialPortInfo::availablePorts()) { m_serialPort->addItem(port.portName()); }
    m_baudRate = new QComboBox(rtuPage); m_baudRate->addItems({QStringLiteral("9600"), QStringLiteral("19200"), QStringLiteral("38400"), QStringLiteral("57600"), QStringLiteral("115200")});
    m_parity = new QComboBox(rtuPage); m_parity->addItems({QStringLiteral("N"), QStringLiteral("E"), QStringLiteral("O")});
    rtuForm->addRow(QStringLiteral("串口"), m_serialPort); rtuForm->addRow(QStringLiteral("波特率"), m_baudRate); rtuForm->addRow(QStringLiteral("校验位"), m_parity);
    m_pages->addWidget(tcpPage); m_pages->addWidget(rtuPage);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addLayout(common); layout->addWidget(m_pages); layout->addWidget(buttons);
    m_type->setCurrentIndex(profile.connectionType == ConnectionType::Tcp ? 0 : 1);
    m_host->setText(profile.tcpHost); m_port->setValue(profile.tcpPort); m_pollInterval->setValue(profile.pollIntervalMs);
    m_serialPort->setCurrentText(profile.serialPort); m_baudRate->setCurrentText(QString::number(profile.baudRate)); m_parity->setCurrentText(QString(profile.parity));
    m_pages->setCurrentIndex(m_type->currentIndex());
    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged), m_pages, &QStackedWidget::setCurrentIndex);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ServerProfile ConnectionConfigDialog::profile() const
{
    ServerProfile output;
    output.connectionType = m_type->currentIndex() == 0 ? ConnectionType::Tcp : ConnectionType::Rtu;
    output.tcpHost = m_host->text().trimmed(); output.tcpPort = quint16(m_port->value());
    output.serialPort = m_serialPort->currentText(); output.baudRate = m_baudRate->currentText().toInt();
    output.parity = m_parity->currentText().at(0); output.pollIntervalMs = m_pollInterval->value();
    return output;
}
