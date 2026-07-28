#ifndef COMM_MONITOR_VIEW_H
#define COMM_MONITOR_VIEW_H

#include "Domain/Models/comm_frame.h"

#include <QPair>
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

signals:
    void monitorPortChanged(const QString &portId);

private slots:
    void onPauseClicked();
    void onClearClicked();
    void onPortComboChanged(int index);
    void flushPendingFrames();

private:
    void appendFrameToTable(const CommFrame &frame);
    void trimExcessRows();

    static const int kMaxRows = 1000;
    static const int kTrimBatch = 200;
    static const int kMaxPending = 2000;
    static const int kFlushIntervalMs = 100;

    QLabel *m_portCaption = nullptr;
    QComboBox *m_portCombo = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QCheckBox *m_autoScroll = nullptr;
    QTableWidget *m_table = nullptr;
    QTimer *m_flushTimer = nullptr;
    QVector<CommFrame> m_pendingFrames;
    bool m_paused = false;
    bool m_updatingPorts = false;
};

#endif
