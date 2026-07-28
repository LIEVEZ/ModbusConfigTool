#ifndef GROUP_CANVAS_VIEW_H
#define GROUP_CANVAS_VIEW_H

#include "Application/Runtime/runtime_service.h"

#include <QHash>
#include <QMap>
#include <QPoint>
#include <QSet>
#include <QStringList>
#include <QWidget>

class GroupCardWidget;
struct ProjectDocument;

class GroupCanvasView : public QWidget
{
    Q_OBJECT

public:
    explicit GroupCanvasView(QWidget *parent = nullptr);
    void setModel(const ProjectDocument &doc,
                  const QHash<QString, RuntimeState> &portStates = {});
    void setSelectedGroup(const QString &groupId);
    QStringList selectedGroupIds() const;
    bool isGroupSelected(const QString &groupId) const;
    void updateRuntimeValue(const ProjectDocument &doc, const QString &pointId);
    void updatePortStates(const QHash<QString, RuntimeState> &portStates);

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
    void applySelectionVisuals();
    void onCardClicked(const QString &groupId, Qt::KeyboardModifiers modifiers);
    void onCardDragStarted(const QString &groupId, const QPoint &offset);
    void onCardDragging(const QString &groupId, const QPoint &globalPos);
    void onCardDragFinished(const QString &groupId);
    bool isPortLive(const QString &portId) const;

    QMap<QString, GroupCardWidget*> m_cards;
    QHash<QString, RuntimeState> m_portStates;
    QSet<QString> m_selectedIds;
    QString m_draggedCardId;
    QPoint m_dragOffset;
    QHash<QString, QPoint> m_dragStartPositions;
};

#endif
