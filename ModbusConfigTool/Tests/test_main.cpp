#include "test_registry.h"

#include <QApplication>
#include <QTest>

QList<TestFactory> &testFactories()
{
    static QList<TestFactory> factories;
    return factories;
}

TestRegistrar::TestRegistrar(const TestFactory &factory)
{
    testFactories().append(factory);
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    int status = 0;
    for (const TestFactory &factory : testFactories())
    {
        QObject *testObject = factory();
        QByteArray executableName = testObject->metaObject()->className();
        char *testArguments[] = {executableName.data(), nullptr};
        int testArgumentCount = 1;
        status |= QTest::qExec(testObject, testArgumentCount, testArguments);
        QTest::qCleanup();
        delete testObject;
    }
    return status;
}
