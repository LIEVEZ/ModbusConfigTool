#ifndef GROUP_REALTIME_PANEL_H
#define GROUP_REALTIME_PANEL_H

#include <QDialog>

class QTableWidget;
struct RegisterGroup;
struct ProjectDocument;

class GroupRealtimePanel : public QDialog
{
    Q_OBJECT

public:
    explicit GroupRealtimePanel(const QString &groupId, const ProjectDocument &doc, QWidget *parent = nullptr);
    void updateValues(const ProjectDocument &doc);

signals:
    void configureRegistersRequested(const QString &groupId);

private:
    QString m_groupId;
    QTableWidget *m_table = nullptr;
};

#endif
