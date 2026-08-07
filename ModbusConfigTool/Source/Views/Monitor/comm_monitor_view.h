#ifndef COMM_MONITOR_VIEW_H
#define COMM_MONITOR_VIEW_H

#include "Domain/Models/comm_frame.h"

#include <QPair>
#include <QPoint>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;

class CommMonitorView : public QWidget
{
    Q_OBJECT

public:
    explicit CommMonitorView(QWidget *parent = nullptr);

    void setPorts(const QVector<QPair<QString, QString>> &ports, const QString &selectedPortId);
    void setSelectedPortId(const QString &portId);
    QString selectedPortId() const;
    void setHint(const QString &text);
    void appendFrame(const CommFrame &frame);
    void clearFrames();
    bool isPaused() const;
    // 把当前表格中的全部消息导出到 path（UTF-8 带 BOM，表头 + 每行一条消息）。
    // 表格为空或写入失败返回 false。
    bool exportToFile(const QString &path) const;

signals:
    void monitorPortChanged(const QString &portId);

private slots:
    void onPauseClicked();
    void onClearClicked();
    void onCopyClicked();
    void onExportClicked();
    void onPortComboChanged(int index);
    void onTableContextMenuRequested(const QPoint &pos);
    void onCellDoubleClicked(int row, int column);
    void onSelectionChanged();
    void flushPendingFrames();

private:
    void appendFrameToTable(const CommFrame &frame);
    void trimExcessRows();
    QString hexTextAtRow(int row) const;
    QString rowTextAt(int row) const;
    QString allHexText() const;
    void copyToClipboard(const QString &text);
    void updateActionStates();

    static const int kMaxRows = 1000;
    static const int kTrimBatch = 200;
    static const int kMaxPending = 2000;
    static const int kFlushIntervalMs = 100;

    QLabel *m_portCaption = nullptr;
    QComboBox *m_portCombo = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QCheckBox *m_autoScroll = nullptr;
    QTableWidget *m_table = nullptr;
    QTimer *m_flushTimer = nullptr;
    QVector<CommFrame> m_pendingFrames;
    bool m_paused = false;
    bool m_updatingPorts = false;
};

#endif
