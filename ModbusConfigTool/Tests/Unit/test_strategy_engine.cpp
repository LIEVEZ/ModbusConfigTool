#include "Infrastructure/Strategy/strategy_engine.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QSignalSpy>
#include <QTest>

class StrategyEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void producesLinearValuesWhileRunning()
    {
        qRegisterMetaType<RegisterValue>();
        RegisterPoint point = ProjectFactory::createRegister(QStringLiteral("group"), 0);
        point.strategy.type = StrategyType::Linear;
        point.strategy.enabled = true;
        point.strategy.parameters = {{QStringLiteral("startValue"), 0.0},
                                     {QStringLiteral("endValue"), 2.0},
                                     {QStringLiteral("step"), 1.0},
                                     {QStringLiteral("intervalMs"), 10}};
        StrategyEngine engine;
        QSignalSpy spy(&engine, &StrategyEngine::valueReady);

        engine.start({point});
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 2, 200);
        engine.stop();

        const RegisterValue first = qvariant_cast<RegisterValue>(spy.at(0).at(1));
        QCOMPARE(first.toUnsigned64(), quint64(1));
    }
};

REGISTER_TEST(StrategyEngineTest);

#include "test_strategy_engine.moc"
