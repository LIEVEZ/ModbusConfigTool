#include "Domain/Values/register_value.h"
#include "test_registry.h"

#include <QTest>

class RegisterValueTest : public QObject
{
    Q_OBJECT

private slots:
    void preservesUnsigned64Precision()
    {
        const QString maximum = QStringLiteral("18446744073709551615");
        const RegisterValue value = RegisterValue::fromUnsigned64(maximum.toULongLong());

        QCOMPARE(value.toStorageString(), maximum);
        QCOMPARE(value.dataType(), DataType::UInt64);
    }

    void rejectsOutOfRangeSigned16()
    {
        const ValueResult result = RegisterValue::fromText(DataType::Int16,
                                                            QStringLiteral("32768"));

        QVERIFY(!result.result.success);
        QCOMPARE(result.result.field, QStringLiteral("value"));
    }

    void keepsFloatingPointType()
    {
        const ValueResult result = RegisterValue::fromText(DataType::Float32,
                                                            QStringLiteral("12.5"));

        QVERIFY(result.result.success);
        QCOMPARE(result.value.dataType(), DataType::Float32);
        QCOMPARE(result.value.toDouble(), 12.5);
    }
};

REGISTER_TEST(RegisterValueTest);

#include "test_register_value.moc"
