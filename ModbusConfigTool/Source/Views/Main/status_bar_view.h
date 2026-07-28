#ifndef STATUS_BAR_VIEW_H
#define STATUS_BAR_VIEW_H

#include "Domain/Models/project_document.h"
#include "Domain/Models/domain_enums.h"

#include <QHash>
#include <QWidget>

class QLabel;

class StatusBarView : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarView(QWidget *parent = nullptr);
    void updateStatus(const ProjectDocument &document,
                      bool dirty,
                      const QHash<QString, RuntimeState> &portStates);
    void showMessage(const QString &message);

private:
    QLabel *m_message = nullptr;
    QLabel *m_runtime = nullptr;
    QLabel *m_connection = nullptr;
    QLabel *m_statistics = nullptr;
    QLabel *m_saved = nullptr;
};

#endif
