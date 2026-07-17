#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include "Domain/Models/register_point.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>

class StrategyEngine : public QObject
{
    Q_OBJECT

public:
    explicit StrategyEngine(QObject *parent = nullptr);

public slots:
    void start(const QList<RegisterPoint> &points);
    void stop();

signals:
    void valueReady(const QString &pointId, const RegisterValue &value);

private slots:
    void processDueItems();

private:
    struct Item
    {
        RegisterPoint point;
        qint64 nextDueMs = 0;
        double linearValue = 0.0;
        int linearDirection = 1;
    };

    int intervalFor(const RegisterPoint &point) const;
    RegisterValue nextValue(Item *item, double elapsedSeconds) const;

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    QHash<QString, Item> m_items;
};

#endif
