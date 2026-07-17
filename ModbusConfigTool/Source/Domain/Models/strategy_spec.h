#ifndef STRATEGY_SPEC_H
#define STRATEGY_SPEC_H

#include "Domain/Models/domain_enums.h"

#include <QVariantMap>

struct StrategySpec
{
    StrategyType type = StrategyType::None;
    bool enabled = false;
    QVariantMap parameters;
};

#endif
