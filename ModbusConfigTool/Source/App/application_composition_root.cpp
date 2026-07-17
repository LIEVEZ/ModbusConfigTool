#include "application_composition_root.h"

#include "ViewModels/Main/main_window_view_model.h"
#include "Views/Main/main_window.h"

ApplicationCompositionRoot::ApplicationCompositionRoot(QObject *parent)
    : QObject(parent),
      m_mainViewModel(new MainWindowViewModel(this))
{
}

MainWindow *ApplicationCompositionRoot::createMainWindow()
{
    return new MainWindow(m_mainViewModel);
}
