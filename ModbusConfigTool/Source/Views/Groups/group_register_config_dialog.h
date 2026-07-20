#ifndef GROUP_REGISTER_CONFIG_DIALOG_H
#define GROUP_REGISTER_CONFIG_DIALOG_H

#include "Domain/Models/project_document.h"

#include <QDialog>

class QTableView;
class RegisterTableModel;

class GroupRegisterConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupRegisterConfigDialog(const QString &groupId, const ProjectDocument &doc, QWidget *parent = nullptr);
    void setDocument(const ProjectDocument &doc);

signals:
    void addRegisterRequested(const QString &groupId);
    void editRegisterRequested(const QString &registerId);
    void removeRegistersRequested(const QStringList &registerIds);
    void importCsvRequested(const QString &groupId);
    void exportCsvRequested(const QString &groupId);

private:
    void setupToolbar();
    void onAddRegister();
    void onEditRegister();
    void onRemoveRegisters();
    void onImportCsv();
    void onExportCsv();

    QString m_groupId;
    ProjectDocument m_filteredDoc;
    QTableView *m_table = nullptr;
    RegisterTableModel *m_model = nullptr;
};

#endif
