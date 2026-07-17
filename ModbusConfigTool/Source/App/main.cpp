#include "Views/Main/main_window.h"
#include "App/application_composition_root.h"

#include <QApplication>
#include <QFile>

namespace
{
QString loadStyleSheet()
{
    const QStringList paths = {
        QStringLiteral(":/styles/base.qss"),
        QStringLiteral(":/styles/controls.qss"),
        QStringLiteral(":/styles/tables.qss"),
        QStringLiteral(":/styles/dialogs.qss")
    };
    QString styleSheet;

    for (const QString &path : paths)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            styleSheet += QString::fromUtf8(file.readAll());
        }
    }
    return styleSheet;
}
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("ModbusConfigTool"));
    application.setOrganizationName(QStringLiteral("LIEVE"));
    application.setStyleSheet(loadStyleSheet());

    ApplicationCompositionRoot compositionRoot;
    MainWindow *window = compositionRoot.createMainWindow();
    window->show();
    const int result = application.exec();
    delete window;
    return result;
}
