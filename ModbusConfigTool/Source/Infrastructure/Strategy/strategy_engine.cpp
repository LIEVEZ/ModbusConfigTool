#include "strategy_engine.h"

#include <QRandomGenerator>
#include <QtMath>

StrategyEngine::StrategyEngine(QObject *parent) : QObject(parent)
{
    m_timer.setInterval(10);
    connect(&m_timer, &QTimer::timeout, this, &StrategyEngine::processDueItems);
}

void StrategyEngine::start(const QList<RegisterPoint> &points)
{
    stop(); m_elapsed.start();
    for (const RegisterPoint &point : points)
    {
        if (!point.strategy.enabled || point.strategy.type == StrategyType::None) { continue; }
        Item item; item.point = point; item.nextDueMs = intervalFor(point);
        item.linearValue = point.strategy.parameters.value(QStringLiteral("startValue"), point.currentValue.toDouble()).toDouble();
        m_items.insert(point.id, item);
    }
    if (!m_items.isEmpty()) { m_timer.start(); }
}

void StrategyEngine::stop()
{
    m_timer.stop(); m_items.clear();
}

void StrategyEngine::processDueItems()
{
    const qint64 now = m_elapsed.elapsed();
    for (Item &item : m_items)
    {
        if (now < item.nextDueMs) { continue; }
        emit valueReady(item.point.id, nextValue(&item, now / 1000.0));
        item.nextDueMs = now + intervalFor(item.point);
    }
}

int StrategyEngine::intervalFor(const RegisterPoint &point) const
{
    return qMax(10, point.strategy.parameters.value(QStringLiteral("intervalMs"), 1000).toInt());
}

RegisterValue StrategyEngine::nextValue(Item *item, double elapsedSeconds) const
{
    const QVariantMap &params = item->point.strategy.parameters;
    double value = item->point.currentValue.toDouble();
    if (item->point.strategy.type == StrategyType::Linear)
    {
        const double start = params.value(QStringLiteral("startValue"), 0.0).toDouble();
        const double end = params.value(QStringLiteral("endValue"), 100.0).toDouble();
        const double step = qAbs(params.value(QStringLiteral("step"), 1.0).toDouble());
        item->linearValue += step * item->linearDirection;
        if (item->linearValue >= end) { item->linearValue = end; item->linearDirection = -1; }
        if (item->linearValue <= start) { item->linearValue = start; item->linearDirection = 1; }
        value = item->linearValue;
    }
    else if (item->point.strategy.type == StrategyType::Random)
    {
        const double minimum = params.value(QStringLiteral("minValue"), 0.0).toDouble();
        const double maximum = params.value(QStringLiteral("maxValue"), 100.0).toDouble();
        value = minimum + QRandomGenerator::global()->generateDouble() * (maximum - minimum);
    }
    else if (item->point.strategy.type == StrategyType::SineWave)
    {
        const double amplitude = params.value(QStringLiteral("amplitude"), 1.0).toDouble();
        const double frequency = params.value(QStringLiteral("frequencyHz"), 1.0).toDouble();
        const double center = params.value(QStringLiteral("center"), 0.0).toDouble();
        value = center + amplitude * qSin(2.0 * M_PI * frequency * elapsedSeconds);
    }
    value = qBound(item->point.minimumValue.toDouble(), value, item->point.maximumValue.toDouble());
    switch (item->point.dataType)
    {
    case DataType::Float32:
    case DataType::Float64: return RegisterValue::fromFloating(value, item->point.dataType);
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64: return RegisterValue::fromSigned64(qRound64(value), item->point.dataType);
    default: return RegisterValue::fromUnsigned64(quint64(qMax(0.0, qRound64(value) * 1.0)), item->point.dataType);
    }
}
