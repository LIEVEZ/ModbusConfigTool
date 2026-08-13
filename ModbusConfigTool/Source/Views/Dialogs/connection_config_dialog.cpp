#include "connection_config_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>

ConnectionConfigDialog::ConnectionConfigDialog(const ConnectionPort &port, QWidget *parent)
    : QDialog(parent)
    , m_originalId(port.id)
{
    const bool isNew = port.id.isEmpty();
    setWindowTitle(isNew ? QStringLiteral("新增端口") : QStringLiteral("编辑端口"));
    resize(480, 360);

    auto *layout = new QVBoxLayout(this);
    auto *common = new QFormLayout;

    m_portName = new QLineEdit(this);
    m_portName->setPlaceholderText(QStringLiteral("例如：主站"));
    m_type = new QComboBox(this);
    m_type->addItems({QStringLiteral("TCP Server"), QStringLiteral("RTU Slave")});
    m_pollInterval = new QSpinBox(this);
    m_pollInterval->setRange(100, 60000);
    m_pollInterval->setSuffix(QStringLiteral(" ms"));

    common->addRow(QStringLiteral("端口名称"), m_portName);
    common->addRow(QStringLiteral("连接类型"), m_type);
    common->addRow(QStringLiteral("轮询周期"), m_pollInterval);

    m_pages = new QStackedWidget(this);

    auto *tcpPage = new QWidget(this);
    auto *tcpForm = new QFormLayout(tcpPage);
    m_host = new QLineEdit(tcpPage);
    m_port = new QSpinBox(tcpPage);
    m_port->setRange(1, 65535);
    tcpForm->addRow(QStringLiteral("主机地址"), m_host);
    tcpForm->addRow(QStringLiteral("端口"), m_port);

    auto *rtuPage = new QWidget(this);
    auto *rtuForm = new QFormLayout(rtuPage);
    m_serialPort = new QComboBox(rtuPage);
    QStringList portNames;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        portNames.append(info.portName());
    }
    std::sort(portNames.begin(), portNames.end(), [](const QString &left, const QString &right) {
        auto comNumber = [](const QString &name) {
            QString digits;
            for (const QChar &ch : name)
            {
                if (ch.isDigit())
                {
                    digits.append(ch);
                }
            }
            bool ok = false;
            const int value = digits.toInt(&ok);
            return ok ? value : -1;
        };
        const int leftNumber = comNumber(left);
        const int rightNumber = comNumber(right);
        if (leftNumber != rightNumber)
        {
            return leftNumber < rightNumber;
        }
        return left < right;
    });
    m_serialPort->addItems(portNames);
    m_baudRate = new QComboBox(rtuPage);
    m_baudRate->addItems({
        QStringLiteral("1200"), QStringLiteral("2400"), QStringLiteral("4800"),
        QStringLiteral("9600"), QStringLiteral("19200"), QStringLiteral("38400"),
        QStringLiteral("57600"), QStringLiteral("115200")
    });
    m_parity = new QComboBox(rtuPage);
    m_parity->addItem(QStringLiteral("无校验"), QStringLiteral("N"));
    m_parity->addItem(QStringLiteral("奇校验"), QStringLiteral("O"));
    m_parity->addItem(QStringLiteral("偶校验"), QStringLiteral("E"));
    rtuForm->addRow(QStringLiteral("串口"), m_serialPort);
    rtuForm->addRow(QStringLiteral("波特率"), m_baudRate);
    rtuForm->addRow(QStringLiteral("校验位"), m_parity);

    m_pages->addWidget(tcpPage);
    m_pages->addWidget(rtuPage);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addLayout(common);
    layout->addWidget(m_pages);
    layout->addWidget(buttons);

    m_portName->setText(port.name);
    m_type->setCurrentIndex(port.profile.connectionType == ConnectionType::Tcp ? 0 : 1);
    m_host->setText(port.profile.tcpHost);
    m_port->setValue(port.profile.tcpPort);
    m_pollInterval->setValue(port.profile.pollIntervalMs);
    m_serialPort->setCurrentText(port.profile.serialPort);
    m_baudRate->setCurrentText(QString::number(port.profile.baudRate));
    const int parityIndex = m_parity->findData(QString(port.profile.parity));
    m_parity->setCurrentIndex(parityIndex >= 0 ? parityIndex : 0);
    m_pages->setCurrentIndex(m_type->currentIndex());

    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
            m_pages, &QStackedWidget::setCurrentIndex);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ConnectionPort ConnectionConfigDialog::port() const
{
    ConnectionPort output;
    output.id = m_originalId;
    output.name = m_portName->text().trimmed();
    output.profile.connectionType = m_type->currentIndex() == 0
        ? ConnectionType::Tcp
        : ConnectionType::Rtu;
    output.profile.tcpHost = m_host->text().trimmed();
    output.profile.tcpPort = quint16(m_port->value());
    output.profile.serialPort = m_serialPort->currentText();
    output.profile.baudRate = m_baudRate->currentText().toInt();
    const QString parity = m_parity->currentData().toString();
    output.profile.parity = parity.isEmpty() ? QLatin1Char('N') : parity.at(0);
    output.profile.pollIntervalMs = m_pollInterval->value();
    return output;
}
