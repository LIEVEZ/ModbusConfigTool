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
    bool enabled = true;        // 停用则运行时跳过本组点位
    QString portId;             // 绑定的连接端口 id；空 = 未绑定
    int canvasX = 0;            // 画布坐标
    int canvasY = 0;
};

#endif
