#ifndef GROUP_CANVAS_VIEW_H
#define GROUP_CANVAS_VIEW_H

#include <QWidget>
#include <QMap>

class GroupCardWidget;
struct RegisterGroup;
struct ProjectDocument;

class GroupCanvasView : public QWidget
{
    Q_OBJECT

public:
    explicit GroupCanvasView(QWidget *parent = nullptr);
    void setModel(const ProjectDocument &doc);
    void setSelectedGroup(const QString &groupId);
    void updateRuntimeValue(const ProjectDocument &doc, const QString &pointId);

signals:
    void groupMoved(const QString &groupId, int x, int y);
    void groupSelected(const QString &groupId);
    void groupEnabledChangeRequested(const QString &groupId, bool enabled);
    void groupPortChangeRequested(const QString &groupId, const QString &portId);
    void groupDoubleClicked(const QString &groupId);
    void groupContextMenuRequested(const QString &groupId, const QPoint &globalPos);
    void canvasClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateCanvasExtent();
    void onCardDragStarted(const QString &groupId, const QPoint &offset);
    void onCardDragging(const QString &groupId, const QPoint &globalPos);
    void onCardDragFinished(const QString &groupId);

    QMap<QString, GroupCardWidget*> m_cards;
    QString m_draggedCardId;
    QPoint m_dragOffset;
};

#endif
