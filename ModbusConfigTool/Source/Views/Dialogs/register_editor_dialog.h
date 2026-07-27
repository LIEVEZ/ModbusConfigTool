#ifndef REGISTER_EDITOR_DIALOG_H
#define REGISTER_EDITOR_DIALOG_H

#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class StrategyEditorWidget;

class RegisterEditorDialog : public QDialog
{
    Q_OBJECT

public:
    RegisterEditorDialog(const RegisterPoint &point,
                         const QList<RegisterGroup> &groups,
                         QWidget *parent = nullptr);

    RegisterPoint point() const;

signals:
    void manualWriteRequested(const RegisterValue &value);

private:
    QWidget *createSectionCard(const QString &title, const QString &subtitle, QWidget *body) const;
    QLabel *createFieldLabel(const QString &text) const;
    QWidget *createField(const QString &label, QWidget *editor) const;
    void refreshDerivedFields();
    void refreshSubtitle();
    void refreshWriteAvailability();

    RegisterPoint m_original;
    QLabel *m_subtitleLabel = nullptr;
    QCheckBox *m_enabled = nullptr;
    QComboBox *m_group = nullptr;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_protocolKey = nullptr;
    QLineEdit *m_category = nullptr;
    QLineEdit *m_label = nullptr;
    QSpinBox *m_slave = nullptr;
    QSpinBox *m_address = nullptr;
    QComboBox *m_storage = nullptr;
    QComboBox *m_dataType = nullptr;
    QComboBox *m_endian = nullptr;
    QLabel *m_registerCount = nullptr;
    QLabel *m_readCode = nullptr;
    QLabel *m_writeCode = nullptr;
    QDoubleSpinBox *m_offset = nullptr;
    QSpinBox *m_precision = nullptr;
    QLineEdit *m_unit = nullptr;
    QLineEdit *m_value = nullptr;
    QLabel *m_unitSuffix = nullptr;
    QPushButton *m_manualWriteButton = nullptr;
    StrategyEditorWidget *m_strategyEditor = nullptr;
};

#endif
