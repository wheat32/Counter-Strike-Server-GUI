#pragma once

#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

#include "appConfig.h"

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
        AppSettings     = 5
    };

    QComboBox*    m_gameSelector         = nullptr;
    QPushButton*  m_startStopBtn         = nullptr;
    QPushButton*  m_serverControlsNavBtn = nullptr;
    QButtonGroup* m_navGroup             = nullptr;
    QStackedWidget* m_pageStack          = nullptr;
    bool          m_serverRunning        = false;

    // Page pointers for direct method calls
    class ServerPage*         m_serverPage         = nullptr;
    class ServerControlsPage* m_serverControlsPage = nullptr;
    class ServerManager*      m_serverManager      = nullptr;

    QPushButton* makeNavButton(const QString& label,
                               const QString& iconResource,
                               Page page);
    void selectPage(Page page);
    void setServerRunning(bool running);
    void onStartStopClicked();
    void updateWindowIcon(AppConfig::Game game);

public:
    // Re-reads AppConfig::selectedGame() and updates the combo box + icon to match.
    // Call after any external code changes the selected game (e.g. ServerSetupDialog).
    void syncGameSelection();
};
