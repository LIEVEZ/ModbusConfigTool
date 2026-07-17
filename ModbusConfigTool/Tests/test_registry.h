#ifndef TEST_REGISTRY_H
#define TEST_REGISTRY_H

#include <functional>

#include <QList>
#include <QObject>

using TestFactory = std::function<QObject *()>;

QList<TestFactory> &testFactories();

class TestRegistrar
{
public:
    explicit TestRegistrar(const TestFactory &factory);
};

#define REGISTER_TEST(TestClass) \
    static TestRegistrar registrar_##TestClass([]() -> QObject * \
    { \
        return new TestClass; \
    })

#endif
