#ifndef REGISTER_PATCH_H
#define REGISTER_PATCH_H

#include "Domain/Models/strategy_spec.h"

#include <QString>

struct RegisterPatch
{
    bool changeGroup = false;
    QString groupId;
    bool changeCategory = false;
    QString category;
    bool changeLabel = false;
    QString label;
    bool changeEnabled = false;
    bool enabled = true;
    bool changeStrategy = false;
    StrategySpec strategy;
};

#endif
