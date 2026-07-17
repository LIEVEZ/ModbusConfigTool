#ifndef RUNTIME_VALUE_VIEW_H
#define RUNTIME_VALUE_VIEW_H

#include "Views/Registers/register_filter_proxy_model.h"
#include "Views/Registers/register_table_model.h"

#include <QWidget>

class QTableView;

class RuntimeValueView : public QWidget
{
    Q_OBJECT

public:
    explicit RuntimeValueView(QWidget *parent = nullptr);
    void setDocument(const ProjectDocument *document);
    void setGroupFilter(const QString &groupName);

signals:
    void locateRequested(const QString &pointId);

private:
    RegisterTableModel *m_model = nullptr;
    RegisterFilterProxyModel *m_proxy = nullptr;
    QTableView *m_table = nullptr;
};

#endif
