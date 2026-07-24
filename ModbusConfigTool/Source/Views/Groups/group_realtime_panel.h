#ifndef GROUP_REALTIME_PANEL_H
#define GROUP_REALTIME_PANEL_H

#include "Domain/Models/project_document.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QTableWidget;

class GroupRealtimePanel : public QDialog
{
    Q_OBJECT

public:
    explicit GroupRealtimePanel(const QString &groupId,
                                const ProjectDocument &doc,
                                QWidget *parent = nullptr);
    void updateValues(const ProjectDocument &doc);

signals:
    void configureRegistersRequested(const QString &groupId);

private:
    void rebuildTable();
    void onSearchTextChanged(const QString &text);
    bool matchesSearch(const RegisterPoint &point) const;

    QString m_groupId;
    ProjectDocument m_document;
    QString m_searchText;
    QTableWidget *m_table = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLabel *m_countBadge = nullptr;
};

#endif
