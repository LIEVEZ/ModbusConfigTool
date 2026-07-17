#ifndef REGISTER_EDITOR_DIALOG_H
#define REGISTER_EDITOR_DIALOG_H

#include "Domain/Models/register_point.h"
#include "Domain/Models/register_group.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class StrategyEditorWidget;

class RegisterEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterEditorDialog(const RegisterPoint &point,
                                  const QList<RegisterGroup> &groups,
                                  QWidget *parent = nullptr);
    RegisterPoint point() const;

signals:
    void manualWriteRequested(const RegisterValue &value);

private:
    RegisterPoint m_original;
    QComboBox *m_group = nullptr;
    QSpinBox *m_slave = nullptr;
    QSpinBox *m_address = nullptr;
    QLineEdit *m_name = nullptr;
    QComboBox *m_dataType = nullptr;
    QComboBox *m_endian = nullptr;
    QComboBox *m_storage = nullptr;
    QLineEdit *m_protocolKey = nullptr;
    QCheckBox *m_enabled = nullptr;
    StrategyEditorWidget *m_strategyEditor = nullptr;
    QLineEdit *m_value = nullptr;
};

#endif
