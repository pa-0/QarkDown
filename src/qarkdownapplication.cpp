#include "qarkdownapplication.h"
#include "logger.h"
#include <QtGui/QFileOpenEvent>
#include <QtGui/QDesktopServices>
#include <QStandardPaths>


struct applicationVersion
{
    int major;
    int minor;
    int tiny;
} appVersion = {0, 4, 3};

#define kCopyrightYearStr "2011-2018"
#define kWebsiteURL "http://hasseg.org/qarkdown"

QarkdownApplication::QarkdownApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
    setQuitOnLastWindowClosed(true);
    mainWindow = NULL;
#ifdef Q_OS_LINUX
    setWindowIcon(QIcon(":/appIcon.png"));
#endif
    QCoreApplication::setApplicationName("QarkDown");
    QCoreApplication::setApplicationVersion(QString("%1.%2.%3").arg(appVersion.major).arg(appVersion.minor).arg(appVersion.tiny));
}

QarkdownApplication::~QarkdownApplication()
{
}

QString QarkdownApplication::copyrightYear()
{
    return kCopyrightYearStr;
}

QString QarkdownApplication::websiteURL()
{
    return kWebsiteURL;
}

QString QarkdownApplication::applicationStoragePath()
{
    QString appName = QCoreApplication::applicationName();
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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

bool QarkdownApplication::copyResourceToFile(QString resourcePath, QString targetFilePath)
{
    QFile source(resourcePath);

    if (QFile::exists(targetFilePath))
    {
        if (!QFile::remove(targetFilePath))
        {
            Logger::warning("Cannot copy resource "+resourcePath
                            +" to file: "+targetFilePath
                            +" -- cannot delete existing file at target path");
            return false;
        }
    }

    if (!source.copy(targetFilePath))
    {
        Logger::warning("Cannot copy resource "+resourcePath
                        +" to file: "+targetFilePath);
        return false;
    }
    return true;
}

bool QarkdownApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen && mainWindow) {
        mainWindow->openFile(static_cast<QFileOpenEvent *>(event)->file());
        return true;
    }

    return QApplication::event(event);
}
