#ifndef REGISTER_CONFIG_VIEW_H
#define REGISTER_CONFIG_VIEW_H

#include "Views/Registers/register_filter_proxy_model.h"
#include "Views/Registers/register_table_model.h"

#include <QWidget>

class QComboBox;
class QLineEdit;
class QTableView;

class RegisterConfigView : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterConfigView(QWidget *parent = nullptr);
    void setDocument(const ProjectDocument *document);
    void setGroupFilter(const QString &groupName);
    QStringList selectedPointIds() const;
    QString currentPointId() const;
    void refreshPoint(const QString &pointId);
    void selectPoint(const QString &pointId);
    void setMappingEditingEnabled(bool enabled);

signals:
    void addRequested();
    void editRequested(const QString &pointId);
    void deleteRequested(const QStringList &pointIds);
    void enableRequested(const QStringList &pointIds, bool enabled);

private:
    RegisterTableModel *m_model = nullptr;
    RegisterFilterProxyModel *m_proxy = nullptr;
    QTableView *m_table = nullptr;
};

#endif
