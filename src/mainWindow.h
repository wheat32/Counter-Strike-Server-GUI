#pragma once

#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

#include "appConfig.h"

#ifdef QT_DEBUG
class DebugPage;
#endif

class MainWindow : public QWidget
{
    Q_OBJECT

    static constexpr int WINDOW_MIN_WIDTH        = 820;
    static constexpr int WINDOW_MIN_HEIGHT       = 520;
    static constexpr int HEADER_HEIGHT           = 56;
    static constexpr int HEADER_H_PADDING        = 16;
    static constexpr int HEADER_V_PADDING        = 10;
    static constexpr int NAV_WIDTH               = 180;
    static constexpr int NAV_V_PADDING           = 8;
    static constexpr int NAV_ICON_SIZE           = 18;
    static constexpr int GAME_ICON_SIZE          = 24;
    static constexpr int GAME_SELECTOR_MIN_WIDTH = 250;

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    enum class Page
    {
        ServerOverview  = 0,
        ServerControls  = 1,
        Maps            = 2,
        Bots            = 3,
        ServerSettings  = 4,
        AppSettings     = 5,
#ifdef QT_DEBUG
        Debug           = 6,
#endif
    };

    QComboBox*             m_gameSelector         = nullptr;
    QPushButton*           m_startStopBtn         = nullptr;
    QPushButton*           m_serverControlsNavBtn = nullptr;
    QPushButton*           m_botsNavBtn           = nullptr;
    QButtonGroup*          m_navGroup             = nullptr;
    QStackedWidget*        m_pageStack            = nullptr;
    QNetworkAccessManager* m_networkManager       = nullptr;
    bool                   m_serverRunning        = false;
    bool                   m_serverStarting       = false;
    bool                   m_restartToastShown    = false;

    // Page pointers for direct method calls
    class ServerPage*         m_serverPage         = nullptr;
    class ServerControlsPage* m_serverControlsPage = nullptr;
    class ServerSettingsPage* m_serverSettingsPage = nullptr;
    class MapsPage*           m_mapsPage           = nullptr;
    class BotsPage*           m_botsPage           = nullptr;
    class ServerManager*      m_serverManager      = nullptr;
#ifdef QT_DEBUG
    DebugPage*                m_debugPage          = nullptr;
#endif

    QPushButton* makeNavButton(const QString& label,
                               const QString& iconResource,
                               Page page);
    void selectPage(Page page);
    void setServerRunning(bool running);
    void onStartStopClicked();
    void updateWindowIcon(AppConfig::Game game);
    void checkForUpdates();
    void checkWhatsNew();
    void onSettingChanged();

public:
    // Re-reads AppConfig::selectedGame() and updates the combo box + icon to match.
    // Call after any external code changes the selected game (e.g. ServerSetupDialog).
    void syncGameSelection();
};
