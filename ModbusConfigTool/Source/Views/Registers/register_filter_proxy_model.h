#ifndef REGISTER_FILTER_PROXY_MODEL_H
#define REGISTER_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class RegisterFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum class SearchMode { All, Name, Address, Category, ProtocolKey };
    explicit RegisterFilterProxyModel(QObject *parent = nullptr);
    void setSearchMode(SearchMode mode);
    void setSearchText(const QString &text);
    void setGroupName(const QString &name);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    SearchMode m_mode = SearchMode::All;
    QString m_text;
    QString m_groupName;
};

#endif
