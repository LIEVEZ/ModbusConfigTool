#include "comm_monitor_view.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QTableWidgetItem *makeItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}
}

CommMonitorView::CommMonitorView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("commMonitorView"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName(QStringLiteral("commMonitorToolBar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    m_portCaption = new QLabel(QStringLiteral("端口"), toolbar);
    m_portCaption->setObjectName(QStringLiteral("commMonitorPortLabel"));

    m_portCombo = new QComboBox(toolbar);
    m_portCombo->setObjectName(QStringLiteral("commMonitorPortCombo"));
    m_portCombo->setMinimumWidth(180);
    m_portCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    m_pauseButton = new QPushButton(QStringLiteral("暂停"), toolbar);
    m_pauseButton->setObjectName(QStringLiteral("commMonitorPauseButton"));
    m_pauseButton->setCheckable(true);
    m_pauseButton->setCursor(Qt::PointingHandCursor);

    m_clearButton = new QPushButton(QStringLiteral("清除"), toolbar);
    m_clearButton->setObjectName(QStringLiteral("commMonitorClearButton"));
    m_clearButton->setCursor(Qt::PointingHandCursor);

    m_autoScroll = new QCheckBox(QStringLiteral("自动滚动"), toolbar);
    m_autoScroll->setObjectName(QStringLiteral("commMonitorAutoScroll"));
    m_autoScroll->setChecked(true);

    toolbarLayout->addWidget(m_portCaption, 0);
    toolbarLayout->addWidget(m_portCombo, 1);
    toolbarLayout->addWidget(m_pauseButton);
    toolbarLayout->addWidget(m_clearButton);
    toolbarLayout->addWidget(m_autoScroll);

    m_hintLabel = new QLabel(QStringLiteral("请选择要监控的连接端口"), this);
    m_hintLabel->setObjectName(QStringLiteral("commMonitorHintLabel"));
    m_hintLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("commMonitorTable"));
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("方向"),
        QStringLiteral("功能码"),
        QStringLiteral("地址"),
        QStringLiteral("数量"),
        QStringLiteral("结果"),
        QStringLiteral("HEX")
    });
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->verticalHeader()->setDefaultSectionSize(22);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->horizontalHeader()->setStretchLastSection(true);
    // Fixed widths avoid ResizeToContents remeasure on every insert.
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 96);
    m_table->setColumnWidth(1, 42);
    m_table->setColumnWidth(2, 54);
    m_table->setColumnWidth(3, 72);
    m_table->setColumnWidth(4, 54);
    m_table->setColumnWidth(5, 72);

    layout->addWidget(toolbar);
    layout->addWidget(m_hintLabel);
    layout->addWidget(m_table, 1);

    m_flushTimer = new QTimer(this);
    m_flushTimer->setObjectName(QStringLiteral("commMonitorFlushTimer"));
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kFlushIntervalMs);
    connect(m_flushTimer, &QTimer::timeout, this, &CommMonitorView::flushPendingFrames);

    connect(m_pauseButton, &QPushButton::clicked, this, &CommMonitorView::onPauseClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &CommMonitorView::onClearClicked);
    connect(m_portCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CommMonitorView::onPortComboChanged);
}

void CommMonitorView::setPorts(const QVector<QPair<QString, QString>> &ports,
                               const QString &preferredPortId)
{
    m_updatingPorts = true;
    const QString previousId = preferredPortId.isEmpty() ? selectedPortId() : preferredPortId;
    m_portCombo->clear();
    m_portCombo->addItem(QStringLiteral("未选择端口"), QString());
    for (const auto &port : ports)
    {
        m_portCombo->addItem(port.second, port.first);
    }
    setSelectedPortId(previousId);
    m_updatingPorts = false;

    if (selectedPortId() != previousId && !previousId.isEmpty())
    {
        emit monitorPortChanged(selectedPortId());
    }
}

void CommMonitorView::setSelectedPortId(const QString &portId)
{
    m_updatingPorts = true;
    int index = m_portCombo->findData(portId);
    if (index < 0)
    {
        index = 0;
    }
    m_portCombo->setCurrentIndex(index);
    m_updatingPorts = false;
}

QString CommMonitorView::selectedPortId() const
{
    return m_portCombo->currentData().toString();
}

void CommMonitorView::setHint(const QString &text)
{
    m_hintLabel->setText(text);
}

bool CommMonitorView::isPaused() const
{
    return m_paused;
}

void CommMonitorView::clearFrames()
{
    m_flushTimer->stop();
    m_pendingFrames.clear();
    m_table->setRowCount(0);
}

void CommMonitorView::appendFrame(const CommFrame &frame)
{
    if (m_paused)
    {
        return;
    }

    if (m_pendingFrames.size() >= kMaxPending)
    {
        const int dropCount = m_pendingFrames.size() - (kMaxPending / 2);
        m_pendingFrames.remove(0, dropCount);
    }

    m_pendingFrames.append(frame);
    if (!m_flushTimer->isActive())
    {
        m_flushTimer->start();
    }
}

void CommMonitorView::flushPendingFrames()
{
    if (m_pendingFrames.isEmpty())
    {
        return;
    }

    QVector<CommFrame> batch;
    batch.swap(m_pendingFrames);

    const bool autoScroll = m_autoScroll->isChecked();
    m_table->setUpdatesEnabled(false);
    m_table->setSortingEnabled(false);

    for (const CommFrame &frame : batch)
    {
        appendFrameToTable(frame);
    }
    trimExcessRows();

    m_table->setUpdatesEnabled(true);
    if (autoScroll && m_table->rowCount() > 0)
    {
        m_table->scrollToBottom();
    }
}

void CommMonitorView::appendFrameToTable(const CommFrame &frame)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    const QString direction = frame.direction == CommDirection::Rx
        ? QStringLiteral("收")
        : QStringLiteral("发");
    const QString addressText = frame.address < 0 ? QStringLiteral("-") : QString::number(frame.address);
    const QString quantityText = frame.quantity < 0 ? QStringLiteral("-") : QString::number(frame.quantity);
    QString resultText = QStringLiteral("-");
    if (!frame.isRequest)
    {
        if (frame.success)
        {
            resultText = QStringLiteral("OK");
        }
        else
        {
            resultText = QStringLiteral("异常%1")
                             .arg(frame.exceptionCode, 2, 16, QLatin1Char('0'))
                             .toUpper();
        }
    }

    m_table->setItem(row, 0, makeItem(frame.timestamp.toString(QStringLiteral("HH:mm:ss.zzz"))));
    m_table->setItem(row, 1, makeItem(direction));
    m_table->setItem(row, 2, makeItem(frame.functionCodeText));
    m_table->setItem(row, 3, makeItem(addressText));
    m_table->setItem(row, 4, makeItem(quantityText));
    m_table->setItem(row, 5, makeItem(resultText));
    m_table->setItem(row, 6, makeItem(CommFrameFactory::formatHex(frame.pduBytes)));
}

void CommMonitorView::trimExcessRows()
{
    // Keep a hysteresis window so we rebuild only every ~kTrimBatch overflow, not every flush.
    if (m_table->rowCount() <= kMaxRows)
    {
        return;
    }

    const int targetRows = kMaxRows - kTrimBatch;
    const int keepFrom = m_table->rowCount() - qMax(targetRows, 1);
    if (keepFrom <= 0)
    {
        return;
    }

    QVector<QList<QTableWidgetItem *>> kept;
    kept.reserve(m_table->rowCount() - keepFrom);
    for (int row = keepFrom; row < m_table->rowCount(); ++row)
    {
        QList<QTableWidgetItem *> items;
        items.reserve(m_table->columnCount());
        for (int col = 0; col < m_table->columnCount(); ++col)
        {
            items.append(m_table->takeItem(row, col));
        }
        kept.append(items);
    }

    m_table->setRowCount(0);
    m_table->setRowCount(kept.size());
    for (int row = 0; row < kept.size(); ++row)
    {
        const QList<QTableWidgetItem *> &items = kept.at(row);
        for (int col = 0; col < items.size(); ++col)
        {
            m_table->setItem(row, col, items.at(col));
        }
    }
}

void CommMonitorView::onPauseClicked()
{
    m_paused = m_pauseButton->isChecked();
    m_pauseButton->setText(m_paused ? QStringLiteral("继续") : QStringLiteral("暂停"));
    if (m_paused)
    {
        m_flushTimer->stop();
        m_pendingFrames.clear();
    }
}

void CommMonitorView::onClearClicked()
{
    clearFrames();
}

void CommMonitorView::onPortComboChanged(int index)
{
    Q_UNUSED(index);
    if (m_updatingPorts)
    {
        return;
    }
    emit monitorPortChanged(selectedPortId());
}

