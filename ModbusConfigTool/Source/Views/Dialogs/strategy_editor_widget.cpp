#include "strategy_editor_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace
{
QDoubleSpinBox *createValueBox(QWidget *parent)
{
    auto *box = new QDoubleSpinBox(parent);
    box->setDecimals(6);
    box->setRange(-1e12, 1e12);
    return box;
}

QSpinBox *createIntervalBox(QWidget *parent)
{
    auto *box = new QSpinBox(parent);
    box->setRange(10, 3600000);
    box->setSuffix(QStringLiteral(" ms"));
    box->setSingleStep(100);
    return box;
}

QFormLayout *createParamForm(QWidget *page)
{
    auto *form = new QFormLayout(page);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return form;
}
}

StrategyEditorWidget::StrategyEditorWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("strategyEditorWidget"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *topRow = new QWidget(this);
    auto *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(12);

    auto *typeField = new QWidget(topRow);
    auto *typeLayout = new QVBoxLayout(typeField);
    typeLayout->setContentsMargins(0, 0, 0, 0);
    typeLayout->setSpacing(6);
    auto *typeLabel = new QLabel(QStringLiteral("策略类型"), typeField);
    typeLabel->setObjectName(QStringLiteral("registerEditorFieldLabel"));
    m_type = new QComboBox(typeField);
    m_type->addItems({
        QStringLiteral("none"),
        QStringLiteral("linear"),
        QStringLiteral("random"),
        QStringLiteral("sine_wave")
    });
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(m_type);

    auto *enableField = new QWidget(topRow);
    auto *enableLayout = new QVBoxLayout(enableField);
    enableLayout->setContentsMargins(0, 0, 0, 0);
    enableLayout->setSpacing(6);
    auto *enableLabel = new QLabel(QStringLiteral("状态"), enableField);
    enableLabel->setObjectName(QStringLiteral("registerEditorFieldLabel"));
    m_enabled = new QCheckBox(QStringLiteral("启用模拟策略"), enableField);
    m_enabled->setObjectName(QStringLiteral("strategyEnableToggle"));
    enableLayout->addWidget(enableLabel);
    enableLayout->addWidget(m_enabled, 0, Qt::AlignLeft | Qt::AlignVCenter);
    enableLayout->addStretch(1);

    topLayout->addWidget(typeField, 1);
    topLayout->addWidget(enableField, 1);

    m_pages = new QStackedWidget(this);
    m_pages->setObjectName(QStringLiteral("strategyEditorPages"));
    m_pages->addWidget(createEmptyPage());
    m_pages->addWidget(createLinearPage());
    m_pages->addWidget(createRandomPage());
    m_pages->addWidget(createSinePage());

    layout->addWidget(topRow);
    layout->addWidget(m_pages);

    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
    {
        m_pages->setCurrentIndex(index);
        refreshState();
    });
    connect(m_enabled, &QCheckBox::toggled, this, [this](bool)
    {
        refreshState();
    });

    refreshState();
}

QWidget *StrategyEditorWidget::createEmptyPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("strategyEmptyPage"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 16, 14, 16);
    layout->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("未启用模拟"), page);
    title->setObjectName(QStringLiteral("strategyEmptyTitle"));
    m_hintLabel = new QLabel(QStringLiteral("选择 linear / random / sine_wave 后可配置参数。\n保存后将保持当前静态值。"), page);
    m_hintLabel->setObjectName(QStringLiteral("strategyEmptyHint"));
    m_hintLabel->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(m_hintLabel);
    layout->addStretch(1);
    return page;
}

QWidget *StrategyEditorWidget::createLinearPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("strategyParamPage"));
    auto *form = createParamForm(page);
    m_linearStart = createValueBox(page);
    m_linearEnd = createValueBox(page);
    m_linearStep = createValueBox(page);
    m_linearStep->setMinimum(0.000001);
    m_linearInterval = createIntervalBox(page);
    form->addRow(QStringLiteral("起始值"), m_linearStart);
    form->addRow(QStringLiteral("结束值"), m_linearEnd);
    form->addRow(QStringLiteral("步长"), m_linearStep);
    form->addRow(QStringLiteral("周期"), m_linearInterval);
    return page;
}

QWidget *StrategyEditorWidget::createRandomPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("strategyParamPage"));
    auto *form = createParamForm(page);
    m_randomMin = createValueBox(page);
    m_randomMax = createValueBox(page);
    m_randomInterval = createIntervalBox(page);
    form->addRow(QStringLiteral("最小值"), m_randomMin);
    form->addRow(QStringLiteral("最大值"), m_randomMax);
    form->addRow(QStringLiteral("周期"), m_randomInterval);
    return page;
}

QWidget *StrategyEditorWidget::createSinePage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("strategyParamPage"));
    auto *form = createParamForm(page);
    m_amplitude = createValueBox(page);
    m_amplitude->setMinimum(0.0);
    m_frequency = createValueBox(page);
    m_frequency->setMinimum(0.000001);
    m_center = createValueBox(page);
    m_sineInterval = createIntervalBox(page);
    form->addRow(QStringLiteral("振幅"), m_amplitude);
    form->addRow(QStringLiteral("频率 Hz"), m_frequency);
    form->addRow(QStringLiteral("中心值"), m_center);
    form->addRow(QStringLiteral("周期"), m_sineInterval);
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
    refreshState();
}

StrategySpec StrategyEditorWidget::strategy() const
{
    StrategySpec output;
    strategyTypeFromString(m_type->currentText(), &output.type);
    output.enabled = m_enabled->isChecked() && output.type != StrategyType::None;
    if (output.type == StrategyType::Linear)
    {
        output.parameters = {
            {QStringLiteral("startValue"), m_linearStart->value()},
            {QStringLiteral("endValue"), m_linearEnd->value()},
            {QStringLiteral("step"), m_linearStep->value()},
            {QStringLiteral("intervalMs"), m_linearInterval->value()}
        };
    }
    else if (output.type == StrategyType::Random)
    {
        output.parameters = {
            {QStringLiteral("minValue"), m_randomMin->value()},
            {QStringLiteral("maxValue"), m_randomMax->value()},
            {QStringLiteral("intervalMs"), m_randomInterval->value()}
        };
    }
    else if (output.type == StrategyType::SineWave)
    {
        output.parameters = {
            {QStringLiteral("amplitude"), m_amplitude->value()},
            {QStringLiteral("frequencyHz"), m_frequency->value()},
            {QStringLiteral("center"), m_center->value()},
            {QStringLiteral("intervalMs"), m_sineInterval->value()}
        };
    }
    return output;
}

void StrategyEditorWidget::refreshState()
{
    const bool isNone = m_type->currentIndex() <= 0;
    m_enabled->setEnabled(!isNone);
    if (isNone)
    {
        m_enabled->setChecked(false);
        m_hintLabel->setText(QStringLiteral("当前为静态值模式。\n选择 linear / random / sine_wave 后可配置自动模拟参数。"));
    }
    else if (!m_enabled->isChecked())
    {
        m_hintLabel->setText(QStringLiteral("已选择策略类型，但尚未启用。\n勾选“启用模拟策略”后才会在运行时生效。"));
    }

    if (m_pages)
    {
        for (int i = 1; i < m_pages->count(); ++i)
        {
            if (QWidget *page = m_pages->widget(i))
            {
                page->setEnabled(!isNone);
            }
        }
    }
}

