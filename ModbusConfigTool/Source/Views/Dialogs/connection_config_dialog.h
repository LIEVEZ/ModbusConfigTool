#ifndef CONNECTION_CONFIG_DIALOG_H
#define CONNECTION_CONFIG_DIALOG_H

#include "Domain/Models/connection_port.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QStackedWidget;

class ConnectionConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionConfigDialog(const ConnectionPort &port, QWidget *parent = nullptr);
    ConnectionPort port() const;

private:
    QLineEdit *m_portName = nullptr;
    QComboBox *m_type = nullptr;
    QStackedWidget *m_pages = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QComboBox *m_serialPort = nullptr;
    QComboBox *m_baudRate = nullptr;
    QComboBox *m_parity = nullptr;
    QSpinBox *m_pollInterval = nullptr;
    QSpinBox *m_slaveAddress = nullptr;
    QString m_originalId;
};

#endif
