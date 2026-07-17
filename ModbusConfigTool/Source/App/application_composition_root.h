#ifndef APPLICATION_COMPOSITION_ROOT_H
#define APPLICATION_COMPOSITION_ROOT_H

#include <QObject>

class MainWindow;
class MainWindowViewModel;

class ApplicationCompositionRoot : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationCompositionRoot(QObject *parent = nullptr);
    MainWindow *createMainWindow();

private:
    MainWindowViewModel *m_mainViewModel = nullptr;
};

#endif
