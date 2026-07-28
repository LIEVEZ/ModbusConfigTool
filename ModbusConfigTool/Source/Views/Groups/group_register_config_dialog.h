#ifndef GROUP_REGISTER_CONFIG_DIALOG_H
#define GROUP_REGISTER_CONFIG_DIALOG_H

#include "Domain/Models/project_document.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QTableView;
class RegisterFilterProxyModel;
class RegisterTableModel;

class GroupRegisterConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupRegisterConfigDialog(const QString &groupId,
                                       const ProjectDocument &doc,
                                       QWidget *parent = nullptr);
    void setDocument(const ProjectDocument &doc);

signals:
    void addRegisterRequested(const QString &groupId);
    void editRegisterRequested(const QString &registerId);
    void removeRegistersRequested(const QStringList &registerIds);
    void importCsvRequested(const QString &groupId);
    void exportCsvRequested(const QString &groupId);

private:
    void refreshHeader(const ProjectDocument &doc);
    void refreshActionButtons();
    void applyColumnVisibility();
    void updateFooterInfo();
    void onAddRegister();
    void onImportCsv();
    void onExportCsv();
    void onSearchTextChanged(const QString &text);
    void editRowAtProxy(int proxyRow);
    void removeRowAtProxy(int proxyRow);

    QString m_groupId;
    ProjectDocument m_filteredDoc;
    QWidget *m_swatch = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_subtitleLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLabel *m_footerInfo = nullptr;
    QTableView *m_table = nullptr;
    RegisterTableModel *m_model = nullptr;
    RegisterFilterProxyModel *m_proxy = nullptr;
};

#endif
