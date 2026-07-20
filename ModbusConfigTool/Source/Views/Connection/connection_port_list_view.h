#ifndef CONNECTION_PORT_LIST_VIEW_H
#define CONNECTION_PORT_LIST_VIEW_H

#include "Application/Runtime/runtime_service.h"

#include <QHash>
#include <QWidget>

class QVBoxLayout;
struct ConnectionPort;
struct RegisterGroup;

class ConnectionPortListView : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPortListView(QWidget *parent = nullptr);
    void setModel(const QList<ConnectionPort> &ports,
                  const QList<RegisterGroup> &groups,
                  const QHash<QString, RuntimeState> &states);
    void updatePortState(const QString &portId, RuntimeState state);

signals:
    void addPortRequested();
    void editPortRequested(const QString &portId);
    void removePortRequested(const QString &portId);
    void startPortRequested(const QString &portId);
    void stopPortRequested(const QString &portId);

private:
    QVBoxLayout *m_itemLayout = nullptr;
};

#endif
