#include "Domain/Values/value_converter.h"
#include "test_registry.h"

#include <QTest>

class ValueConverterTest : public QObject
{
    Q_OBJECT

private slots:
    void convertsUnsigned32BigEndian()
    {
        const RegisterValue value = RegisterValue::fromUnsigned64(0x12345678,
                                                                   DataType::UInt32);
        const ConversionResult result = ValueConverter::toRegisters(value, Endian::Big);

        QVERIFY(result.result.success);
        QCOMPARE(result.registers, QVector<quint16>({0x1234, 0x5678}));
    }

    void roundTripsFloatLittleSwap()
    {
        const RegisterValue source = RegisterValue::fromFloating(12.5, DataType::Float32);
        const ConversionResult encoded = ValueConverter::toRegisters(source,
                                                                      Endian::LittleSwap);
        const ValueResult decoded = ValueConverter::fromRegisters(DataType::Float32,
                                                                   Endian::LittleSwap,
                                                                   encoded.registers);

        QVERIFY(decoded.result.success);
        QCOMPARE(decoded.value.toDouble(), 12.5);
    }
};

REGISTER_TEST(ValueConverterTest);

#include "test_value_converter.moc"
