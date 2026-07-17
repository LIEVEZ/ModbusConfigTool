#ifndef STRATEGY_EDITOR_WIDGET_H
#define STRATEGY_EDITOR_WIDGET_H

#include "Domain/Models/strategy_spec.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QStackedWidget;

class StrategyEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyEditorWidget(QWidget *parent = nullptr);
    void setStrategy(const StrategySpec &strategy);
    StrategySpec strategy() const;

private:
    QWidget *createLinearPage();
    QWidget *createRandomPage();
    QWidget *createSinePage();

    QComboBox *m_type = nullptr;
    QCheckBox *m_enabled = nullptr;
    QStackedWidget *m_pages = nullptr;
    QDoubleSpinBox *m_linearStart = nullptr;
    QDoubleSpinBox *m_linearEnd = nullptr;
    QDoubleSpinBox *m_linearStep = nullptr;
    QSpinBox *m_linearInterval = nullptr;
    QDoubleSpinBox *m_randomMin = nullptr;
    QDoubleSpinBox *m_randomMax = nullptr;
    QSpinBox *m_randomInterval = nullptr;
    QDoubleSpinBox *m_amplitude = nullptr;
    QDoubleSpinBox *m_frequency = nullptr;
    QDoubleSpinBox *m_center = nullptr;
    QSpinBox *m_sineInterval = nullptr;
};

#endif
