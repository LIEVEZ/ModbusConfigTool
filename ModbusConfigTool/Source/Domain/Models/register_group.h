#ifndef REGISTER_GROUP_H
#define REGISTER_GROUP_H

#include <QString>

struct RegisterGroup
{
    QString id;
    QString name;
    QString color = QStringLiteral("#f54e00");
    QString description;
    bool isDefault = false;
};

#endif
