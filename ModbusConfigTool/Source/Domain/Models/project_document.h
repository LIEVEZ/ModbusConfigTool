#ifndef PROJECT_DOCUMENT_H
#define PROJECT_DOCUMENT_H

#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QDateTime>
#include <QList>
#include <QSize>

struct ProjectMetadata
{
    QString name = QStringLiteral("未命名工程");
    QString description;
    QDateTime createdAt;
    QDateTime updatedAt;
};

struct UiState
{
    QSize windowSize = QSize(1440, 900);
    QString selectedGroupId;
    int portColWidth = 250;
    int logColWidth = 300;
    QList<int> horizontalSplitterSizes;
    QList<int> verticalSplitterSizes;
};

struct ProjectDocument
{
    int schemaVersion = 2;
    ProjectMetadata project;
    QList<ConnectionPort> ports;
    QList<RegisterGroup> groups;
    QList<RegisterPoint> registers;
    UiState uiState;
};

#endif
