#ifndef RUNTIME_VALUE_VIEW_H
#define RUNTIME_VALUE_VIEW_H

#include "Views/Registers/register_filter_proxy_model.h"
#include "Views/RuntimeValues/runtime_value_table_model.h"

#include <QWidget>
#include <QSet>

class QTableView;

class RuntimeValueView : public QWidget
{
    Q_OBJECT

public:
    explicit RuntimeValueView(QWidget *parent = nullptr);
    void setDocument(const ProjectDocument *document);
    void setGroupFilter(const QString &groupName);
    void queuePointRefresh(const QString &pointId);

signals:
    void locateRequested(const QString &pointId);

private:
    RuntimeValueTableModel *m_model = nullptr;
    RegisterFilterProxyModel *m_proxy = nullptr;
    QTableView *m_table = nullptr;
    QSet<QString> m_pendingPointIds;
};

#endif
