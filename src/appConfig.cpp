#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include "appConfig.h"
#include "debug.h"

namespace
{
QString configDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + QStringLiteral("/CSServerManager");
}
QString configFile() { return configDir() + QStringLiteral("/app.json"); }

constexpr int DEFAULT_PORT = 27015;
} // namespace

AppConfig& AppConfig::instance()
{
    static AppConfig inst;
    return inst;
}

AppConfig::AppConfig()
{
    load();
}

void AppConfig::load()
{
    QFile f(configFile());
    if (f.open(QIODevice::ReadOnly) == false)
        return;

    const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    f.close();

    const QString themeStr = obj.value(QStringLiteral("theme")).toString(QStringLiteral("system"));
    if (themeStr == QStringLiteral("dark"))
    {
        m_theme = Theme::Dark;
    }
    else if (themeStr == QStringLiteral("light"))
    {
        m_theme = Theme::Light;
    }
    else
    {
        m_theme = Theme::System;
    }

    const QString gameStr = obj.value(QStringLiteral("selected_game")).toString(QStringLiteral("cz"));
    m_selectedGame = (gameStr == QStringLiteral("cs16")) ? Game::CS16 : Game::CZ;

    m_cs16ServerPath = obj.value(QStringLiteral("cs16_server_path")).toString();
    m_czServerPath   = obj.value(QStringLiteral("cz_server_path")).toString();

    m_cs16Ip   = obj.value(QStringLiteral("cs16_ip")).toString(QStringLiteral("127.0.0.1"));
    m_czIp     = obj.value(QStringLiteral("cz_ip")).toString(QStringLiteral("127.0.0.1"));
    m_cs16Port     = obj.value(QStringLiteral("cs16_port")).toInt(DEFAULT_PORT);
    m_czPort       = obj.value(QStringLiteral("cz_port")).toInt(DEFAULT_PORT);
    m_cs16StartMap = obj.value(QStringLiteral("cs16_start_map")).toString();
    m_czStartMap   = obj.value(QStringLiteral("cz_start_map")).toString();

    m_cs16MaxPlayers = obj.value(QStringLiteral("cs16_max_players")).toInt(20);
    m_czMaxPlayers   = obj.value(QStringLiteral("cz_max_players")).toInt(20);

    DBG_SETTINGS(QStringLiteral("Config loaded from: ") + configFile());
    DBG_SETTINGS(QStringLiteral("  theme            = ") + themeStr);
    DBG_SETTINGS(QStringLiteral("  selected_game    = ") + gameStr);
    DBG_SETTINGS(QStringLiteral("  cs16_server_path = ") + m_cs16ServerPath);
    DBG_SETTINGS(QStringLiteral("  cz_server_path   = ") + m_czServerPath);
    DBG_SETTINGS(QStringLiteral("  cs16_ip          = ") + m_cs16Ip);
    DBG_SETTINGS(QStringLiteral("  cz_ip            = ") + m_czIp);
    DBG_SETTINGS(QStringLiteral("  cs16_port        = ") + QString::number(m_cs16Port));
    DBG_SETTINGS(QStringLiteral("  cz_port          = ") + QString::number(m_czPort));
}

bool AppConfig::save() const
{
    const QDir dir;
    if (dir.mkpath(configDir()) == false)
        return false;

    QJsonObject obj;

    QString themeStr;
    switch (m_theme)
    {
        case Theme::Dark:
            themeStr = QStringLiteral("dark");
            break;
        case Theme::Light:
            themeStr = QStringLiteral("light");
            break;
        default:
            themeStr = QStringLiteral("system");
            break;
    }
    obj[QStringLiteral("theme")] = themeStr;

    obj[QStringLiteral("selected_game")]    = (m_selectedGame == Game::CS16)
                                              ? QStringLiteral("cs16")
                                              : QStringLiteral("cz");
    obj[QStringLiteral("cs16_server_path")] = m_cs16ServerPath;
    obj[QStringLiteral("cz_server_path")]   = m_czServerPath;
    obj[QStringLiteral("cs16_ip")]          = m_cs16Ip;
    obj[QStringLiteral("cz_ip")]            = m_czIp;
    obj[QStringLiteral("cs16_port")]        = m_cs16Port;
    obj[QStringLiteral("cz_port")]          = m_czPort;
    obj[QStringLiteral("cs16_start_map")]       = m_cs16StartMap;
    obj[QStringLiteral("cz_start_map")]         = m_czStartMap;
    obj[QStringLiteral("cs16_max_players")] = m_cs16MaxPlayers;
    obj[QStringLiteral("cz_max_players")]   = m_czMaxPlayers;

    QFile f(configFile());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text) == false)
        return false;

    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

AppConfig::Theme AppConfig::theme() const { return m_theme; }

void AppConfig::setTheme(const Theme value)
{
    if (m_theme == value) return;
    m_theme = value;
    (void)save();
}

AppConfig::Game AppConfig::selectedGame() const { return m_selectedGame; }

void AppConfig::setSelectedGame(const Game value)
{
    if (m_selectedGame == value) return;
    m_selectedGame = value;
    (void)save();
}

QString AppConfig::cs16ServerPath() const { return m_cs16ServerPath; }

void AppConfig::setCs16ServerPath(const QString& path)
{
    if (m_cs16ServerPath == path) return;
    DBG_SETTINGS(QStringLiteral("Setting changed: cs16_server_path = ") + path);
    m_cs16ServerPath = path;
    (void)save();
}

QString AppConfig::czServerPath() const { return m_czServerPath; }

void AppConfig::setCzServerPath(const QString& path)
{
    if (m_czServerPath == path) return;
    DBG_SETTINGS(QStringLiteral("Setting changed: cz_server_path = ") + path);
    m_czServerPath = path;
    (void)save();
}

QString AppConfig::cs16Ip() const { return m_cs16Ip; }

void AppConfig::setCs16Ip(const QString& ip)
{
    if (m_cs16Ip == ip) return;
    DBG_SETTINGS(QStringLiteral("Setting changed: cs16_ip = ") + ip);
    m_cs16Ip = ip;
    (void)save();
}

QString AppConfig::czIp() const { return m_czIp; }

void AppConfig::setCzIp(const QString& ip)
{
    if (m_czIp == ip) return;
    DBG_SETTINGS(QStringLiteral("Setting changed: cz_ip = ") + ip);
    m_czIp = ip;
    (void)save();
}

int AppConfig::cs16Port() const { return m_cs16Port; }

void AppConfig::setCs16Port(const int port)
{
    if (m_cs16Port == port) return;
    DBG_SETTINGS(QStringLiteral("Setting changed: cs16_port = ") + QString::number(port));
    m_cs16Port = port;
    (void)save();
}

int AppConfig::czPort() const { return m_czPort; }

void AppConfig::setCzPort(const int port)
{
    if (m_czPort == port) return;
    DBG_SETTINGS(QStringLiteral("Setting changed: cz_port = ") + QString::number(port));
    m_czPort = port;
    (void)save();
}

// ── Gameplay settings ─────────────────────────────────────────────────────────

#define IMPL_INT_SETTING(CapGame, lcGame, Field, JsonKey) \
int  AppConfig::lcGame##Field() const { return m_##lcGame##Field; } \
void AppConfig::set##CapGame##Field(const int value) \
{ \
    if (m_##lcGame##Field == value) return; \
    m_##lcGame##Field = value; \
    (void)save(); \
}

#define IMPL_BOOL_SETTING(CapGame, lcGame, Field, JsonKey) \
bool AppConfig::lcGame##Field() const { return m_##lcGame##Field; } \
void AppConfig::set##CapGame##Field(const bool value) \
{ \
    if (m_##lcGame##Field == value) return; \
    m_##lcGame##Field = value; \
    (void)save(); \
}

IMPL_INT_SETTING(Cs16, cs16, MaxPlayers, cs16_max_players)
IMPL_INT_SETTING(Cz,   cz,   MaxPlayers, cz_max_players)

#undef IMPL_INT_SETTING
#undef IMPL_BOOL_SETTING

// ── Map selection ─────────────────────────────────────────────────────────────

QString AppConfig::cs16StartMap() const { return m_cs16StartMap; }

void AppConfig::setCs16StartMap(const QString& map)
{
    if (m_cs16StartMap == map) return;
    m_cs16StartMap = map;
    (void)save();
}

QString AppConfig::czStartMap() const { return m_czStartMap; }

void AppConfig::setCzStartMap(const QString& map)
{
    if (m_czStartMap == map) return;
    m_czStartMap = map;
    (void)save();
}

void AppConfig::resetToDefaults()
{
    DBG_SETTINGS(QStringLiteral("AppConfig::resetToDefaults()"));
    QFile::remove(configFile());
    m_theme          = Theme::System;
    m_selectedGame   = Game::CZ;
    m_cs16ServerPath = QString();
    m_czServerPath   = QString();
    m_cs16Ip         = QStringLiteral("127.0.0.1");
    m_czIp           = QStringLiteral("127.0.0.1");
    m_cs16Port       = DEFAULT_PORT;
    m_czPort         = DEFAULT_PORT;
    m_cs16StartMap      = QString();
    m_czStartMap        = QString();
    m_cs16MaxPlayers = 20;
    m_czMaxPlayers   = 20;
}
