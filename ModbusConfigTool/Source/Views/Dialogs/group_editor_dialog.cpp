#include "group_editor_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

static const QStringList PALETTE = {
    QStringLiteral("#f54e00"), QStringLiteral("#1f8a65"), QStringLiteral("#2f6feb"),
    QStringLiteral("#c08532"), QStringLiteral("#8b5cf6"), QStringLiteral("#cf2d56")
};

GroupEditorDialog::GroupEditorDialog(const RegisterGroup &group, QWidget *parent)
    : QDialog(parent), m_group(group), m_selectedColor(group.color)
{
    setWindowTitle(group.id.isEmpty() ? QStringLiteral("新增分组") : QStringLiteral("编辑分组"));
    resize(420, 320);

    auto *layout = new QVBoxLayout(this);

    // Name
    auto *nameLabel = new QLabel(QStringLiteral("分组名称"), this);
    m_nameEdit = new QLineEdit(group.name, this);
    m_nameEdit->setPlaceholderText(QStringLiteral("例如:电能参数"));
    m_nameEdit->setMaxLength(40);

    // Color swatches
    auto *colorLabel = new QLabel(QStringLiteral("颜色标记"), this);
    m_swatchContainer = new QWidget(this);
    auto *swatchLayout = new QHBoxLayout(m_swatchContainer);
    swatchLayout->setContentsMargins(0, 0, 0, 0);
    swatchLayout->setSpacing(8);
    setupColorSwatches();

    // Description
    auto *descLabel = new QLabel(QStringLiteral("描述 (可选)"), this);
    m_descEdit = new QTextEdit(group.description, this);
    m_descEdit->setPlaceholderText(QStringLiteral("备注该分组的用途"));
    m_descEdit->setMaximumHeight(80);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    layout->addWidget(nameLabel);
    layout->addWidget(m_nameEdit);
    layout->addSpacing(8);
    layout->addWidget(colorLabel);
    layout->addWidget(m_swatchContainer);
    layout->addSpacing(8);
    layout->addWidget(descLabel);
    layout->addWidget(m_descEdit);
    layout->addStretch();
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_nameEdit->setFocus();
}

RegisterGroup GroupEditorDialog::group() const
{
    RegisterGroup result = m_group;
    result.name = m_nameEdit->text().trimmed();
    result.color = m_selectedColor;
    result.description = m_descEdit->toPlainText().trimmed();
    return result;
}

void GroupEditorDialog::setupColorSwatches()
{
    auto *swatchLayout = qobject_cast<QHBoxLayout*>(m_swatchContainer->layout());
    if (!swatchLayout) return;

    // Clear existing
    while (QLayoutItem *item = swatchLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    for (const QString &color : PALETTE)
    {
        auto *swatch = new QPushButton(m_swatchContainer);
        swatch->setFixedSize(32, 32);
        swatch->setStyleSheet(QString("QPushButton { background-color: %1; border: 2px solid %2; border-radius: 6px; }"
                                       "QPushButton:hover { border-color: #000; }")
                              .arg(color, color == m_selectedColor ? "#000" : "transparent"));
        swatch->setCursor(Qt::PointingHandCursor);

        connect(swatch, &QPushButton::clicked, this, [this, color]()
        {
            m_selectedColor = color;
            setupColorSwatches();
        });

        swatchLayout->addWidget(swatch);
    }
    swatchLayout->addStretch();
}
