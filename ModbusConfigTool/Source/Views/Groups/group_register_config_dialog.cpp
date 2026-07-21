#include "group_register_config_dialog.h"

#include "Domain/Models/project_document.h"
#include "Views/Registers/register_filter_proxy_model.h"
#include "Views/Registers/register_table_model.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
const int kActionColumn = 13;

QPushButton *makeRowButton(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedHeight(26);
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  padding: 2px 9px;"
        "  font-size: 12px;"
        "  border-radius: 6px;"
        "  background: #f7f7f4;"
        "  border: 1px solid rgba(38,37,30,40);"
        "}"
        "QPushButton:hover { border-color: #f54e00; }"));
    return button;
}
}

GroupRegisterConfigDialog::GroupRegisterConfigDialog(const QString &groupId,
                                                     const ProjectDocument &doc,
                                                     QWidget *parent)
    : QDialog(parent), m_groupId(groupId)
{
    setObjectName(QStringLiteral("groupRegisterConfigDialog"));
    setWindowTitle(QStringLiteral("寄存器配置"));
    resize(960, 640);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("groupConfigHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 12);
    headerLayout->setSpacing(12);

    m_swatch = new QWidget(header);
    m_swatch->setObjectName(QStringLiteral("groupConfigSwatch"));
    m_swatch->setFixedSize(6, 30);

    auto *titleBox = new QVBoxLayout;
    titleBox->setContentsMargins(0, 0, 0, 0);
    titleBox->setSpacing(2);
    m_titleLabel = new QLabel(header);
    m_titleLabel->setObjectName(QStringLiteral("groupConfigTitle"));
    m_subtitleLabel = new QLabel(header);
    m_subtitleLabel->setObjectName(QStringLiteral("groupConfigSubtitle"));
    titleBox->addWidget(m_titleLabel);
    titleBox->addWidget(m_subtitleLabel);

    headerLayout->addWidget(m_swatch, 0, Qt::AlignTop);
    headerLayout->addLayout(titleBox, 1);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName(QStringLiteral("groupConfigToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(4, 8, 4, 8);
    toolbarLayout->setSpacing(8);

    auto *addButton = new QPushButton(QStringLiteral("＋ 新增寄存器"), toolbar);
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *importButton = new QPushButton(QStringLiteral("导入 CSV"), toolbar);
    auto *exportButton = new QPushButton(QStringLiteral("导出 CSV"), toolbar);
    m_searchEdit = new QLineEdit(toolbar);
    m_searchEdit->setObjectName(QStringLiteral("groupConfigSearch"));
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索名称、地址或协议键..."));
    m_searchEdit->setClearButtonEnabled(true);

    toolbarLayout->addWidget(addButton);
    toolbarLayout->addWidget(importButton);
    toolbarLayout->addWidget(exportButton);
    toolbarLayout->addWidget(m_searchEdit, 1);

    m_table = new QTableView(this);
    m_table->setObjectName(QStringLiteral("groupRegisterTable"));
    m_model = new RegisterTableModel(this);
    m_proxy = new RegisterFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSearchMode(RegisterFilterProxyModel::SearchMode::All);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QTableView::SelectRows);
    m_table->setSelectionMode(QTableView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setShowGrid(false);
    m_table->setMouseTracking(true);

    auto *footer = new QWidget(this);
    footer->setObjectName(QStringLiteral("groupConfigFooter"));
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(4, 12, 4, 0);
    m_footerInfo = new QLabel(footer);
    m_footerInfo->setObjectName(QStringLiteral("groupConfigFooterInfo"));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, footer);
    auto *closeButton = buttons->button(QDialogButtonBox::Close);
    if (closeButton)
    {
        closeButton->setObjectName(QStringLiteral("primaryButton"));
        closeButton->setText(QStringLiteral("关闭"));
    }
    footerLayout->addWidget(m_footerInfo);
    footerLayout->addStretch();
    footerLayout->addWidget(buttons);

    layout->addWidget(header);
    layout->addWidget(toolbar);
    layout->addWidget(m_table, 1);
    layout->addWidget(footer);

    connect(addButton, &QPushButton::clicked, this, &GroupRegisterConfigDialog::onAddRegister);
    connect(importButton, &QPushButton::clicked, this, &GroupRegisterConfigDialog::onImportCsv);
    connect(exportButton, &QPushButton::clicked, this, &GroupRegisterConfigDialog::onExportCsv);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &GroupRegisterConfigDialog::onSearchTextChanged);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &index)
    {
        if (index.isValid())
        {
            editRowAtProxy(index.row());
        }
    });

    setDocument(doc);
}

void GroupRegisterConfigDialog::setDocument(const ProjectDocument &doc)
{
    m_filteredDoc = doc;
    m_filteredDoc.registers.erase(
        std::remove_if(m_filteredDoc.registers.begin(),
                       m_filteredDoc.registers.end(),
                       [this](const RegisterPoint &point)
        {
            return point.groupId != m_groupId;
        }),
        m_filteredDoc.registers.end());

    refreshHeader(doc);
    if (m_model)
    {
        m_model->setDocument(&m_filteredDoc);
    }
    applyColumnVisibility();
    refreshActionButtons();
    updateFooterInfo();
}

void GroupRegisterConfigDialog::refreshHeader(const ProjectDocument &doc)
{
    const RegisterGroup *group = nullptr;
    for (const RegisterGroup &candidate : doc.groups)
    {
        if (candidate.id == m_groupId)
        {
            group = &candidate;
            break;
        }
    }

    const QString name = group ? group->name : QStringLiteral("未知分组");
    const QString color = group ? group->color : QStringLiteral("#f54e00");
    const bool enabled = group ? group->enabled : true;
    const QString description = group && !group->description.isEmpty()
        ? group->description
        : QStringLiteral("无描述");

    m_titleLabel->setText(name);
    m_subtitleLabel->setText(QStringLiteral("%1 · %2")
                                 .arg(enabled ? QStringLiteral("启用") : QStringLiteral("已停用"),
                                      description));
    m_swatch->setStyleSheet(QStringLiteral(
        "background: %1; border-radius: 3px;").arg(color));
    setWindowTitle(QStringLiteral("%1 · 寄存器配置").arg(name));
}

void GroupRegisterConfigDialog::applyColumnVisibility()
{
    // 原型列：从站/地址/数量/名称/类型/协议键/操作
    static const QList<int> visibleColumns = {1, 2, 3, 4, 5, 8, kActionColumn};
    for (int column = 0; column < m_model->columnCount(); ++column)
    {
        m_table->setColumnHidden(column, !visibleColumns.contains(column));
    }
    m_table->setColumnWidth(1, 70);
    m_table->setColumnWidth(2, 80);
    m_table->setColumnWidth(3, 60);
    m_table->setColumnWidth(4, 160);
    m_table->setColumnWidth(5, 90);
    m_table->setColumnWidth(8, 140);
    m_table->setColumnWidth(kActionColumn, 140);
}

void GroupRegisterConfigDialog::refreshActionButtons()
{
    if (!m_proxy || !m_model)
    {
        return;
    }

    for (int row = 0; row < m_proxy->rowCount(); ++row)
    {
        const QModelIndex actionIndex = m_proxy->index(row, kActionColumn);
        if (!actionIndex.isValid())
        {
            continue;
        }

        const QModelIndex sourceIndex = m_proxy->mapToSource(m_proxy->index(row, 0));
        const QString pointId = m_model->pointId(sourceIndex.row());
        if (pointId.isEmpty())
        {
            continue;
        }

        auto *container = new QWidget(m_table);
        container->setObjectName(QStringLiteral("groupConfigRowActions"));
        auto *rowLayout = new QHBoxLayout(container);
        rowLayout->setContentsMargins(4, 2, 4, 2);
        rowLayout->setSpacing(6);

        auto *editButton = makeRowButton(QStringLiteral("编辑"),
                                         QStringLiteral("groupConfigEditButton"),
                                         container);
        auto *deleteButton = makeRowButton(QStringLiteral("删除"),
                                           QStringLiteral("groupConfigDeleteButton"),
                                           container);
        deleteButton->setProperty("dangerAction", true);
        deleteButton->setStyleSheet(deleteButton->styleSheet()
                                    + QStringLiteral("QPushButton { color: #cf2d56; }"));

        rowLayout->addWidget(editButton);
        rowLayout->addWidget(deleteButton);
        rowLayout->addStretch();

        connect(editButton, &QPushButton::clicked, this, [this, pointId]()
        {
            emit editRegisterRequested(pointId);
        });
        connect(deleteButton, &QPushButton::clicked, this, [this, pointId]()
        {
            emit removeRegistersRequested(QStringList{pointId});
        });

        m_table->setIndexWidget(actionIndex, container);
    }
}

void GroupRegisterConfigDialog::updateFooterInfo()
{
    const int total = m_filteredDoc.registers.size();
    const int visible = m_proxy ? m_proxy->rowCount() : total;
    if (m_searchEdit && !m_searchEdit->text().trimmed().isEmpty() && visible != total)
    {
        m_footerInfo->setText(QStringLiteral("显示 %1 / 共 %2 条寄存器")
                                  .arg(visible)
                                  .arg(total));
    }
    else
    {
        m_footerInfo->setText(QStringLiteral("共 %1 条寄存器").arg(total));
    }
}

void GroupRegisterConfigDialog::onAddRegister()
{
    emit addRegisterRequested(m_groupId);
}

void GroupRegisterConfigDialog::onImportCsv()
{
    emit importCsvRequested(m_groupId);
}

void GroupRegisterConfigDialog::onExportCsv()
{
    emit exportCsvRequested(m_groupId);
}

void GroupRegisterConfigDialog::onSearchTextChanged(const QString &text)
{
    if (m_proxy)
    {
        m_proxy->setSearchText(text);
    }
    refreshActionButtons();
    updateFooterInfo();
}

void GroupRegisterConfigDialog::editRowAtProxy(int proxyRow)
{
    if (!m_proxy || !m_model || proxyRow < 0 || proxyRow >= m_proxy->rowCount())
    {
        return;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(m_proxy->index(proxyRow, 0));
    const QString pointId = m_model->pointId(sourceIndex.row());
    if (!pointId.isEmpty())
    {
        emit editRegisterRequested(pointId);
    }
}

void GroupRegisterConfigDialog::removeRowAtProxy(int proxyRow)
{
    if (!m_proxy || !m_model || proxyRow < 0 || proxyRow >= m_proxy->rowCount())
    {
        return;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(m_proxy->index(proxyRow, 0));
    const QString pointId = m_model->pointId(sourceIndex.row());
    if (!pointId.isEmpty())
    {
        emit removeRegistersRequested(QStringList{pointId});
    }
}
