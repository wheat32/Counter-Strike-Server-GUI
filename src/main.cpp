#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QStandardPaths>
#include <QSysInfo>
#include <QWidget>

#include "appConfig.h"
#include "cli/platformUtils.h"
#include "debug.h"
#include "themeManager.h"

namespace
{
constexpr int MAIN_WINDOW_WIDTH  = 900;
constexpr int MAIN_WINDOW_HEIGHT = 600;
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("CSServerManager"));
    QApplication::setApplicationDisplayName(QStringLiteral("CS Server Manager"));
    QApplication::setOrganizationName(QStringLiteral("wheat32"));

    // Read version from embedded version.json — single source of truth.
    QString appVersion = QStringLiteral("unknown");
    QFile vf(QStringLiteral(":/version.json"));
    if (vf.open(QIODevice::ReadOnly))
    {
        const QJsonObject obj = QJsonDocument::fromJson(vf.readAll()).object();
        vf.close();
        if (obj.contains(QStringLiteral("app_version")))
        {
            appVersion = obj[QStringLiteral("app_version")].toString();
        }
    }
    QApplication::setApplicationVersion(appVersion);

    DBG_APP(QStringLiteral("=== CS Server Manager starting ==="));
    DBG_APP(QStringLiteral("App version  : ") + appVersion);
    DBG_APP(QStringLiteral("Qt version   : ") + QString::fromLatin1(qVersion()));
    DBG_APP(QStringLiteral("Package type : ") + packageTypeName());
    DBG_APP(QStringLiteral("OS           : ") + QSysInfo::prettyProductName());
    DBG_APP(QStringLiteral("Kernel       : ") + QSysInfo::kernelVersion());
    DBG_APP(QStringLiteral("CPU arch     : ") + QSysInfo::currentCpuArchitecture());
    DBG_APP(QStringLiteral("Locale       : ") + QLocale::system().name());
    DBG_APP(QStringLiteral("Config dir   : ") + QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    DBG_APP(QStringLiteral("=========================================="));

    ThemeManager::apply(AppConfig::instance().theme());

    // Placeholder window — main window implementation comes next.
    QWidget window;
    window.setWindowTitle(QStringLiteral("CS Server Manager v") + appVersion);
    window.resize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    window.show();

    return QApplication::exec();
}
