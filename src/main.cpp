#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>
#include <QSysInfo>

#include "appConfig.h"
#include "cli/appImageUtils.h"
#include "cli/platformUtils.h"
#include "debug.h"
#include "dialogs/gameServerTypeDialog.h"
#include "mainWindow.h"
#include "serverFiles.h"
#include "serverSetupDialog.h"
#include "serverUtils.h"
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

    // Set the application icon based on the persisted game selection.
    // CS:CZ gets the gold icon; CS 1.6 (and the .desktop default) gets the white icon.
    const QString startupIcon = (AppConfig::instance().selectedGame() == AppConfig::Game::CZ)
        ? QStringLiteral(":/assets/app-icon-cz.svg")
        : QStringLiteral(":/assets/app-icon.svg");
    QApplication::setWindowIcon(QIcon(startupIcon));


    // ── AppImage: detect co-located server ───────────────────────────────────
    if (isRunningAsAppImage())
    {
        const QString appImageFile = qEnvironmentVariable("APPIMAGE");
        const QString appImageDir  = QFileInfo(appImageFile).absolutePath();

        if (isValidServerPath(appImageDir))
        {
            const bool neitherPathSet = AppConfig::instance().cs16ServerPath().isEmpty()
                                     && AppConfig::instance().czServerPath().isEmpty();

            if (neitherPathSet)
            {
                // First launch co-located with hlds_run — ask the user which game.
                GameServerTypeDialog gameTypeDlg;
                gameTypeDlg.exec();
                const AppConfig::Game chosen = gameTypeDlg.selectedGame();
                DBG_APP(QStringLiteral("User identified server as: ")
                        + (chosen == AppConfig::Game::CZ ? QStringLiteral("CZ") : QStringLiteral("CS16")));
                AppConfig::instance().setSelectedGame(chosen);
                AppConfig::instance().setCzServerPath(appImageDir);
                AppConfig::instance().setCs16ServerPath(appImageDir);
            }
            else
            {
                DBG_APP(QStringLiteral("AppImage co-located with hlds_run — auto-configuring unset paths."));
                if (AppConfig::instance().cs16ServerPath().isEmpty())
                    AppConfig::instance().setCs16ServerPath(appImageDir);
                if (AppConfig::instance().czServerPath().isEmpty())
                    AppConfig::instance().setCzServerPath(appImageDir);
            }
        }
    }

    // ── Show main window ──────────────────────────────────────────────────────
    MainWindow window;
    window.resize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    window.show();

    // ── Validate current game path ────────────────────────────────────────────
    // If the selected game has no valid server path, block the main window with
    // the setup dialog. The user must provide a valid path or quit.
    const AppConfig::Game currentGame = AppConfig::instance().selectedGame();
    const QString currentPath = (currentGame == AppConfig::Game::CZ)
        ? AppConfig::instance().czServerPath()
        : AppConfig::instance().cs16ServerPath();

    if (isValidServerPath(currentPath) == false)
    {
        DBG_APP(QStringLiteral("Server path not valid — showing setup dialog."));
        ServerSetupDialog setupDlg(currentGame, &window);
        setupDlg.exec();
        if (setupDlg.userWantsToQuit())
        {
            return 0;
        }
        // Sync the main window in case the dialog switched the selected game.
        window.syncGameSelection();
    }

    // ── Ensure default config files exist for the active game ────────────────
    {
        const AppConfig::Game activeGame = AppConfig::instance().selectedGame();
        const QString activePath = (activeGame == AppConfig::Game::CZ)
            ? AppConfig::instance().czServerPath()
            : AppConfig::instance().cs16ServerPath();

        if (isValidServerPath(activePath))
        {
            ServerFiles::ensureServerConfig(activeGame);
            ServerFiles::ensureMotd(activeGame);
        }
    }

    return QApplication::exec();
}
