#include "batch_edit_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

BatchEditDialog::BatchEditDialog(int count,
                                 const QList<RegisterGroup> &groups,
                                 QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("批量编辑")); resize(480, 330);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("将修改 %1 条寄存器，仅应用已勾选字段。").arg(count), this));
    auto *form = new QFormLayout;
    m_changeGroup = new QCheckBox(QStringLiteral("修改分组"), this); m_group = new QComboBox(this);
    for (const RegisterGroup &group : groups) { m_group->addItem(group.name, group.id); }
    m_changeCategory = new QCheckBox(QStringLiteral("修改分类"), this); m_category = new QLineEdit(this);
    m_changeLabel = new QCheckBox(QStringLiteral("修改标签"), this); m_label = new QLineEdit(this);
    m_changeEnabled = new QCheckBox(QStringLiteral("修改启用状态"), this); m_enabled = new QComboBox(this);
    m_enabled->addItems({QStringLiteral("启用"), QStringLiteral("停用")});
    form->addRow(m_changeGroup, m_group); form->addRow(m_changeCategory, m_category);
    form->addRow(m_changeLabel, m_label); form->addRow(m_changeEnabled, m_enabled);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    layout->addLayout(form); layout->addStretch(); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

RegisterPatch BatchEditDialog::patch() const
{
    RegisterPatch output;
    output.changeGroup = m_changeGroup->isChecked(); output.groupId = m_group->currentData().toString();
    output.changeCategory = m_changeCategory->isChecked(); output.category = m_category->text().trimmed();
    output.changeLabel = m_changeLabel->isChecked(); output.label = m_label->text().trimmed();
    output.changeEnabled = m_changeEnabled->isChecked(); output.enabled = m_enabled->currentIndex() == 0;
    return output;
}
