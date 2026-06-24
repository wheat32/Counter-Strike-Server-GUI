#pragma once

#include <QString>

// AppConfig – persists app-level preferences to ~/.config/CSServerManager/app.json
class AppConfig
{
public:
    enum class Theme { System, Dark, Light };
    enum class Game  { CS16, CZ };

    static AppConfig& instance();

    // Theme
    Theme theme() const;
    void  setTheme(Theme value);

    // Selected game
    Game   selectedGame() const;
    void   setSelectedGame(Game value);

    // Server installation paths (directory containing hlds_run)
    QString cs16ServerPath() const;
    QString czServerPath() const;
    void    setCs16ServerPath(const QString& path);
    void    setCzServerPath(const QString& path);

    // Per-game server IP address
    QString cs16Ip() const;
    QString czIp() const;
    void    setCs16Ip(const QString& ip);
    void    setCzIp(const QString& ip);

    // Per-game server port
    int  cs16Port() const;
    int  czPort() const;
    void setCs16Port(int port);
    void setCzPort(int port);

    // Per-game last-selected starting map
    QString cs16StartMap() const;
    QString czStartMap() const;
    void    setCs16StartMap(const QString& map);
    void    setCzStartMap(const QString& map);

    // Per-game gameplay settings (persisted in AppConfig)
    int  cs16MaxPlayers() const;
    int  czMaxPlayers() const;
    void setCs16MaxPlayers(int value);
    void setCzMaxPlayers(int value);

    void resetToDefaults();

private:
    AppConfig();
    void load();
    bool save() const;

    Theme   m_theme          = Theme::System;
    Game    m_selectedGame   = Game::CZ;
    QString m_cs16ServerPath;
    QString m_czServerPath;
    QString m_cs16Ip         = QStringLiteral("127.0.0.1");
    QString m_czIp           = QStringLiteral("127.0.0.1");
    int     m_cs16Port       = 27015;
    int     m_czPort         = 27015;
    QString m_cs16StartMap;
    QString m_czStartMap;

    int  m_cs16MaxPlayers = 20;
    int  m_czMaxPlayers   = 20;
};
