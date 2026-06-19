#include "app/application.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication qtApplication(argc, argv);
    pyraqt::app::Application application(qtApplication);

    application.initialize();
    application.show();

    const int result = QApplication::exec();
    application.shutdown();
    return result;
}
