#include "comm_monitor_dialog.h"

#include "Views/Monitor/comm_monitor_view.h"

#include <QVBoxLayout>

CommMonitorDialog::CommMonitorDialog(QWidget *parent) : QDialog(parent)
{
    setObjectName(QStringLiteral("commMonitorDialog"));
    setWindowTitle(QStringLiteral("通信监控 - 未选择端口"));
    setWindowFlags(windowFlags() | Qt::Window);
    setModal(false);
    resize(980, 460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_view = new CommMonitorView(this);
    layout->addWidget(m_view);
}

CommMonitorView *CommMonitorDialog::view() const
{
    return m_view;
}
