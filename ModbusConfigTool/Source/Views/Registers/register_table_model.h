#ifndef REGISTER_TABLE_MODEL_H
#define REGISTER_TABLE_MODEL_H

#include "Domain/Models/project_document.h"

#include <QAbstractTableModel>

class RegisterTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RegisterTableModel(QObject *parent = nullptr);
    void setDocument(const ProjectDocument *document);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QString pointId(int row) const;

private:
    QString groupName(const QString &groupId) const;
    const ProjectDocument *m_document = nullptr;
};

#endif
