#ifndef BATCH_EDIT_DIALOG_H
#define BATCH_EDIT_DIALOG_H

#include "Application/Registers/register_patch.h"
#include "Domain/Models/register_group.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;

class BatchEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchEditDialog(int count,
                             const QList<RegisterGroup> &groups,
                             QWidget *parent = nullptr);
    RegisterPatch patch() const;

private:
    QCheckBox *m_changeGroup = nullptr;
    QComboBox *m_group = nullptr;
    QCheckBox *m_changeCategory = nullptr;
    QLineEdit *m_category = nullptr;
    QCheckBox *m_changeLabel = nullptr;
    QLineEdit *m_label = nullptr;
    QCheckBox *m_changeEnabled = nullptr;
    QComboBox *m_enabled = nullptr;
};

#endif
