#ifndef GROUP_REALTIME_PANEL_H
#define GROUP_REALTIME_PANEL_H

#include "Domain/Models/project_document.h"
#include "Domain/Values/register_value.h"

#include <QDialog>
#include <QList>
#include <QPair>

class QLabel;
class QLineEdit;
class QTableWidget;
class QTableWidgetItem;

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
    void editRegisterRequested(const QString &registerId);
    void valueWriteRequested(const QString &pointId, const RegisterValue &value);
    void bulkValuesWriteRequested(const QList<QPair<QString, RegisterValue>> &values);

private slots:
    void onItemChanged(QTableWidgetItem *item);
    void onCellDoubleClicked(int row, int column);
    void onSearchTextChanged(const QString &text);

private:
    void rebuildTable();
    bool tryUpdateValueCells();
    bool matchesSearch(const RegisterPoint &point) const;
    const RegisterPoint *findPoint(const QString &pointId) const;
    QString pointIdAtRow(int row) const;
    QList<RegisterPoint *> collectTargetPoints(bool filteredOnly);
    void applyGeneratedValues(bool filteredOnly, bool resetToZero);
    static RegisterValue randomValueInRange(const RegisterPoint &point);
    static RegisterValue resetValueInRange(const RegisterPoint &point);

    static const int kProtocolKeyColumn = 6;
    static const int kValueColumn = 9;

    QString m_groupId;
    ProjectDocument m_document;
    QString m_searchText;
    QTableWidget *m_table = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLabel *m_countBadge = nullptr;
    bool m_updatingTable = false;
};

#endif
