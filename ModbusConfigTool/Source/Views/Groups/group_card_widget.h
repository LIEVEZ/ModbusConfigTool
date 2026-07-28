#ifndef GROUP_CARD_WIDGET_H
#define GROUP_CARD_WIDGET_H

#include <QList>
#include <QWidget>

class QComboBox;
class QAbstractItemView;
class QLabel;
class QPushButton;
class GroupHoverTip;
struct ConnectionPort;
struct RegisterGroup;
struct RegisterPoint;

class GroupCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GroupCardWidget(const RegisterGroup &group,
                             int registerCount,
                             const QList<ConnectionPort> &ports,
                             const QList<RegisterPoint> &points,
                             bool portLive = false,
                             QWidget *parent = nullptr);
    ~GroupCardWidget() override;

    QString groupId() const { return m_groupId; }
    QString hoverSummaryText() const { return m_hoverSummaryText; }
    QString boundPortId() const;
    void setSelected(bool selected);
    void setPortLive(bool live);
    void updateRegisterCount(int count);
    void updateRuntimeSummary(const QList<RegisterPoint> &points);

signals:
    void dragStarted(const QString &groupId, const QPoint &offset);
    void dragging(const QString &groupId, const QPoint &globalPos);
    void dragFinished(const QString &groupId);
    void clicked(const QString &groupId);
    void doubleClicked(const QString &groupId);
    void contextMenuRequested(const QString &groupId, const QPoint &globalPos);
    void enabledChangeRequested(const QString &groupId, bool enabled);
    void portChangeRequested(const QString &groupId, const QString &portId);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    bool isInteractiveChild(QWidget *widget) const;
    void queuePortChange(const QString &portId);
    void emitPendingPortChange();
    void refreshStyle();
    void rebuildHoverContent(const QList<RegisterPoint> &points);
    void showHoverTip(const QPoint &globalPos);
    void hideHoverTip();
    void applyPortLiveStyle();
    void applyDraggingVisual(bool dragging);
    void restoreIdleGraphicsEffect();

    QString m_groupId;
    QString m_name;
    QString m_description;
    QString m_hoverSummaryText;
    int m_registerCount = 0;
    bool m_groupEnabled = true;
    bool m_selected = false;
    bool m_portLive = false;
    bool m_dragging = false;
    bool m_pressedForDrag = false;
    bool m_controlInteraction = false;
    bool m_hovering = false;
    QPoint m_dragStartPos;
    QString m_pendingPortId;
    bool m_portChangePending = false;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_enabledButton = nullptr;
    QLabel *m_portLiveDot = nullptr;
    QComboBox *m_portCombo = nullptr;
    QAbstractItemView *m_portView = nullptr;
    QWidget *m_portViewport = nullptr;
    GroupHoverTip *m_hoverTip = nullptr;
};

#endif
