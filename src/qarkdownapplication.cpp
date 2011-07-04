#include "qarkdownapplication.h"
#include <QFileOpenEvent>
#include <QDebug>
#include <QDesktopServices>

struct applicationVersion
{
    int major;
    int minor;
    int tiny;
} appVersion = {0, 1, 1};

QarkdownApplication::QarkdownApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
    mainWindow = NULL;
#ifdef Q_OS_LINUX
    setWindowIcon(QIcon(":/appIcon.png"));
#endif
    QCoreApplication::setApplicationName("QarkDown");
    QCoreApplication::setApplicationVersion(QString().sprintf("%i.%i.%i", appVersion.major, appVersion.minor, appVersion.tiny));
}

QString QarkdownApplication::applicationStoragePath()
{
    QString appName = QCoreApplication::applicationName();
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // DataLocation should be defined on all but embedded platforms but just
    // to be safe we do this:
    if (path.isEmpty())
        path = QDir::homePath() + "/." + appName;

    // Let's make sure the path exists
    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }

    return path;
}

bool QarkdownApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen && mainWindow) {
        mainWindow->openFile(static_cast<QFileOpenEvent *>(event)->file());
        return true;
    }

    return QApplication::event(event);
}
