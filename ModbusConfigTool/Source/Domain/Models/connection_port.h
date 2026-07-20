#ifndef CONNECTION_PORT_H
#define CONNECTION_PORT_H

#include "Domain/Models/server_profile.h"

#include <QString>

struct ConnectionPort
{
    QString id;
    QString name;
    ServerProfile profile;   // 复用现有全部连接参数
};

#endif
