#ifndef GROUP_EDITOR_DIALOG_H
#define GROUP_EDITOR_DIALOG_H

#include "Domain/Models/register_group.h"

#include <QDialog>

class QLineEdit;
class QTextEdit;

class GroupEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditorDialog(const RegisterGroup &group, QWidget *parent = nullptr);

    RegisterGroup group() const;

private:
    void setupColorSwatches();

    RegisterGroup m_group;
    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_descEdit = nullptr;
    QWidget *m_swatchContainer = nullptr;
    QString m_selectedColor;
};

#endif
