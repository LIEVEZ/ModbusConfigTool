#include "strategy_editor_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace
{
QDoubleSpinBox *createValueBox(QWidget *parent)
{
    auto *box = new QDoubleSpinBox(parent);
    box->setRange(-1.0e12, 1.0e12);
    box->setDecimals(6);
    return box;
}

QSpinBox *createIntervalBox(QWidget *parent)
{
    auto *box = new QSpinBox(parent);
    box->setRange(10, 60000);
    box->setSuffix(QStringLiteral(" ms"));
    return box;
}
}

StrategyEditorWidget::StrategyEditorWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *common = new QFormLayout;
    m_type = new QComboBox(this);
    m_type->addItems({QStringLiteral("none"), QStringLiteral("linear"),
                      QStringLiteral("random"), QStringLiteral("sine_wave")});
    m_enabled = new QCheckBox(QStringLiteral("启用模拟策略"), this);
    common->addRow(QStringLiteral("策略类型"), m_type);
    common->addRow(QString(), m_enabled);
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(new QWidget(this));
    m_pages->addWidget(createLinearPage());
    m_pages->addWidget(createRandomPage());
    m_pages->addWidget(createSinePage());
    layout->addLayout(common);
    layout->addWidget(m_pages);
    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
            m_pages, &QStackedWidget::setCurrentIndex);
}

QWidget *StrategyEditorWidget::createLinearPage()
{
    auto *page = new QWidget(this); auto *form = new QFormLayout(page);
    m_linearStart = createValueBox(page); m_linearEnd = createValueBox(page);
    m_linearStep = createValueBox(page); m_linearStep->setMinimum(0.000001);
    m_linearInterval = createIntervalBox(page);
    form->addRow(QStringLiteral("起始值"), m_linearStart); form->addRow(QStringLiteral("结束值"), m_linearEnd);
    form->addRow(QStringLiteral("步长"), m_linearStep); form->addRow(QStringLiteral("周期"), m_linearInterval);
    return page;
}

QWidget *StrategyEditorWidget::createRandomPage()
{
    auto *page = new QWidget(this); auto *form = new QFormLayout(page);
    m_randomMin = createValueBox(page); m_randomMax = createValueBox(page);
    m_randomInterval = createIntervalBox(page);
    form->addRow(QStringLiteral("最小值"), m_randomMin); form->addRow(QStringLiteral("最大值"), m_randomMax);
    form->addRow(QStringLiteral("周期"), m_randomInterval); return page;
}

QWidget *StrategyEditorWidget::createSinePage()
{
    auto *page = new QWidget(this); auto *form = new QFormLayout(page);
    m_amplitude = createValueBox(page); m_amplitude->setMinimum(0.0);
    m_frequency = createValueBox(page); m_frequency->setMinimum(0.000001);
    m_center = createValueBox(page); m_sineInterval = createIntervalBox(page);
    form->addRow(QStringLiteral("振幅"), m_amplitude); form->addRow(QStringLiteral("频率 Hz"), m_frequency);
    form->addRow(QStringLiteral("中心值"), m_center); form->addRow(QStringLiteral("周期"), m_sineInterval);
    return page;
}

void StrategyEditorWidget::setStrategy(const StrategySpec &strategy)
{
    m_type->setCurrentText(strategyTypeToString(strategy.type));
    m_enabled->setChecked(strategy.enabled);
    const QVariantMap &p = strategy.parameters;
    m_linearStart->setValue(p.value(QStringLiteral("startValue"), 0.0).toDouble());
    m_linearEnd->setValue(p.value(QStringLiteral("endValue"), 100.0).toDouble());
    m_linearStep->setValue(p.value(QStringLiteral("step"), 1.0).toDouble());
    m_linearInterval->setValue(p.value(QStringLiteral("intervalMs"), 1000).toInt());
    m_randomMin->setValue(p.value(QStringLiteral("minValue"), 0.0).toDouble());
    m_randomMax->setValue(p.value(QStringLiteral("maxValue"), 100.0).toDouble());
    m_randomInterval->setValue(p.value(QStringLiteral("intervalMs"), 1000).toInt());
    m_amplitude->setValue(p.value(QStringLiteral("amplitude"), 1.0).toDouble());
    m_frequency->setValue(p.value(QStringLiteral("frequencyHz"), 1.0).toDouble());
    m_center->setValue(p.value(QStringLiteral("center"), 0.0).toDouble());
    m_sineInterval->setValue(p.value(QStringLiteral("intervalMs"), 100).toInt());
}

StrategySpec StrategyEditorWidget::strategy() const
{
    StrategySpec output;
    strategyTypeFromString(m_type->currentText(), &output.type);
    output.enabled = m_enabled->isChecked() && output.type != StrategyType::None;
    if (output.type == StrategyType::Linear)
    {
        output.parameters = {{QStringLiteral("startValue"), m_linearStart->value()},
            {QStringLiteral("endValue"), m_linearEnd->value()}, {QStringLiteral("step"), m_linearStep->value()},
            {QStringLiteral("intervalMs"), m_linearInterval->value()}};
    }
    else if (output.type == StrategyType::Random)
    {
        output.parameters = {{QStringLiteral("minValue"), m_randomMin->value()},
            {QStringLiteral("maxValue"), m_randomMax->value()}, {QStringLiteral("intervalMs"), m_randomInterval->value()}};
    }
    else if (output.type == StrategyType::SineWave)
    {
        output.parameters = {{QStringLiteral("amplitude"), m_amplitude->value()},
            {QStringLiteral("frequencyHz"), m_frequency->value()}, {QStringLiteral("center"), m_center->value()},
            {QStringLiteral("intervalMs"), m_sineInterval->value()}};
    }
    return output;
}
