#pragma once

#include <QWidget>
#include "appConfig.h"
#include "firewallChecker.h"

class QAction;
class QCheckBox;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class NumberSpinner;
class ToggleWithStatus;

class ServerPage : public QWidget
{
    Q_OBJECT
public:
    explicit ServerPage(QWidget* parent = nullptr);

    // Reload IP, port, and server files for the given game.
    void loadForGame(AppConfig::Game game);

    // Update the map combo to reflect a selection made on the Maps page.
    void setStartMap(const QString& map);

    // Current UI values — used by MainWindow to build the hlds_run command line.
    [[nodiscard]] QString currentIp()   const;
    [[nodiscard]] int     currentPort() const;
    [[nodiscard]] QString currentMap()  const;
    [[nodiscard]] int     maxPlayers()  const;

signals:
    void settingChanged();

protected:
    void changeEvent(QEvent* event) override;

private:
    // Server identity
    QLineEdit*       m_hostnameEdit      = nullptr;
    QLineEdit*       m_passwordEdit      = nullptr;
    QTabWidget*      m_motdTabs          = nullptr;
    QPlainTextEdit*  m_motdEdit          = nullptr;

    // Password reveal toggle
    QAction*         m_passwordToggleAction = nullptr;

    // Tracked "last loaded" values — writes are skipped if nothing changed
    QString          m_loadedHostname;
    QString          m_loadedPassword;
    QString          m_loadedMotd;

    // Connection
    QLineEdit*        m_ipEdit            = nullptr;
    QPushButton*      m_detectIpBtn       = nullptr;
    NumberSpinner*    m_portSpinner       = nullptr;
    QPushButton*      m_firewallCheckBtn  = nullptr;
    QLabel*           m_firewallLabel     = nullptr;
    QLabel*           m_firewallHelpLink  = nullptr;
    FirewallChecker*  m_firewallChecker   = nullptr;
    FirewallChecker::FirewallType m_lastFirewallType = FirewallChecker::FirewallType::Unknown;

    // Gameplay
    QComboBox*       m_mapCombo          = nullptr;
    NumberSpinner*   m_maxPlayersSpinner = nullptr;
    NumberSpinner*   m_timeLimitSpinner  = nullptr;

    // Bots (hidden entirely for CS 1.6)
    QWidget*          m_botsGroup        = nullptr;
    ToggleWithStatus* m_botsToggle       = nullptr;
    QWidget*          m_botOptions       = nullptr;
    NumberSpinner*    m_botCountSpinner  = nullptr;
    QCheckBox*        m_ctBotsCheck      = nullptr;
    QCheckBox*        m_tBotsCheck       = nullptr;

    void refreshPasswordToggleIcon();
    void writeBotsTeamToConfig();
    void detectLocalIp();
    void onFirewallResult(int port, FirewallChecker::Status status, FirewallChecker::FirewallType type);
};
