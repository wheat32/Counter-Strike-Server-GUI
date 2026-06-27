#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "appConfig.h"
#include "debug.h"

// Utilities for reading and writing CS server files (motd.txt, server.cfg).
// All functions are stateless — the game path is looked up from AppConfig at call time.
namespace ServerFiles
{

// Returns the game content directory (czero/ or cstrike/) for the given game.
inline QString gameDirectory(const AppConfig::Game game)
{
    const QString serverPath = (game == AppConfig::Game::CZ)
        ? AppConfig::instance().czServerPath()
        : AppConfig::instance().cs16ServerPath();
    const QString subDir = (game == AppConfig::Game::CZ)
        ? QStringLiteral("czero")
        : QStringLiteral("cstrike");
    return serverPath + u'/' + subDir;
}

// Extracts the value of a key from a single server.cfg line.
// Returns an empty string if the line doesn't match or is a comment.
inline QString extractConfigValue(const QString& line, const QString& key)
{
    const QString trimmed = line.trimmed();

    if (trimmed.startsWith(QStringLiteral("//")))
        return QString();

    if (trimmed.startsWith(key) == false)
        return QString();

    // Key must be followed by whitespace, not just a prefix of a longer token
    if (trimmed.length() == key.length() || trimmed[key.length()].isSpace() == false)
        return QString();

    QString rest = trimmed.mid(key.length()).trimmed();

    // Strip inline comment
    const int commentPos = rest.indexOf(QStringLiteral("//"));
    if (commentPos >= 0)
        rest = rest.left(commentPos).trimmed();

    // Quoted value:  hostname "Nick's Server"
    if (rest.isEmpty() == false && rest[0] == u'"')
    {
        const int closeQuote = rest.indexOf(u'"', 1);
        if (closeQuote > 0)
            return rest.mid(1, closeQuote - 1);
        return rest.mid(1); // unclosed quote — take remainder
    }

    // Unquoted value: take up to the first whitespace
    int i = 0;
    while (i < rest.length() && rest[i].isSpace() == false) { ++i; }
    return rest.left(i);
}

// ── Default content ──────────────────────────────────────────────────────────

// Returns the full text written to a freshly-created server.cfg.
// Values match the ServerConfig struct defaults so the UI and the file agree.
inline QString defaultServerConfigContent()
{
    return QStringLiteral(
        "// CS Server Manager — generated default configuration\n"
        "\n"
        "hostname \"CS Server\"\n"
        "sv_password \"\"\n"
        "sv_lan 0\n"
        "sv_region 0\n"
        "\n"
        "mp_timelimit 0\n"
        "mp_roundtime 5\n"
        "mp_freezetime 6\n"
        "\n"
        "mp_flashlight 1\n"
        "mp_footsteps 1\n"
        "mp_friendlyfire 0\n"
        "mp_autoteambalance 1\n"
        "mp_limitteams 2\n"
        "mp_tkpunish 1\n"
        "mp_hostagepenalty 5\n"
        "\n"
        "sv_maxspeed 320\n"
        "sv_cheats 0\n"
        "sv_aim 1\n"
        "pausable 0\n"
        "sv_pausable 0\n"
        "\n"
        "exec listip.cfg\n"
        "exec banned.cfg\n"
        "\n"
        "// Bot configuration\n"
        "bot_quota 0\n"
        "bot_join_team \"any\"\n"
        "bot_quota_mode fill\n"
        "bot_difficulty 0\n"
        "bot_chatter minimal\n"
        "bot_defer_to_human 0\n"
        "bot_prefix \"\"\n"
        "bot_join_after_player 0\n"
        "bot_auto_vacate 1\n"
        "\n"
        "// Bot allowed weapons\n"
        "bot_allow_pistols 1\n"
        "bot_allow_shotguns 1\n"
        "bot_allow_sub_machine_guns 1\n"
        "bot_allow_rifles 1\n"
        "bot_allow_snipers 1\n"
        "bot_allow_machine_guns 1\n"
        "bot_allow_grenades 1\n"
        "bot_allow_shield 0\n");
}

// Creates server.cfg with sensible defaults if it does not already exist.
// Returns true if the file already existed or was successfully created.
inline bool ensureServerConfig(const AppConfig::Game game)
{
    const QString dir  = gameDirectory(game);
    const QString path = dir + QStringLiteral("/server.cfg");

    if (QFile::exists(path))
        return true;

    if (QDir(dir).exists() == false)
    {
        DBG_APP(QStringLiteral("ServerFiles::ensureServerConfig: game directory not found: ") + dir);
        return false;
    }

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles::ensureServerConfig: could not create ") + path);
        return false;
    }

    QTextStream(&f) << defaultServerConfigContent();
    DBG_APP(QStringLiteral("ServerFiles::ensureServerConfig: created ") + path);
    return true;
}

// Creates motd.txt with a placeholder if it does not already exist.
// Returns true if the file already existed or was successfully created.
inline bool ensureMotd(const AppConfig::Game game)
{
    const QString dir  = gameDirectory(game);
    const QString path = dir + QStringLiteral("/motd.txt");

    if (QFile::exists(path))
        return true;

    if (QDir(dir).exists() == false)
    {
        DBG_APP(QStringLiteral("ServerFiles::ensureMotd: game directory not found: ") + dir);
        return false;
    }

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles::ensureMotd: could not create ") + path);
        return false;
    }

    QTextStream(&f) << QStringLiteral("Welcome to this Counter-Strike server!\n");
    DBG_APP(QStringLiteral("ServerFiles::ensureMotd: created ") + path);
    return true;
}

// ── MOTD ─────────────────────────────────────────────────────────────────────

// Reads motd.txt from the game directory. Returns an empty string if not found.
inline QString readMotd(const AppConfig::Game game)
{
    const QString path = gameDirectory(game) + QStringLiteral("/motd.txt");
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: could not open ") + path);
        return QString();
    }
    DBG_APP(QStringLiteral("ServerFiles: read motd.txt from ") + path);
    return QTextStream(&f).readAll();
}

// Writes content to motd.txt, creating it with a default first if needed.
inline bool writeMotd(const AppConfig::Game game, const QString& content)
{
    ensureMotd(game);

    const QString dir  = gameDirectory(game);
    const QString path = dir + QStringLiteral("/motd.txt");

    if (QDir(dir).exists() == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: game directory not found: ") + dir);
        return false;
    }

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: could not write ") + path);
        return false;
    }

    QTextStream out(&f);
    out << content;
    DBG_APP(QStringLiteral("ServerFiles: wrote motd.txt"));
    return true;
}

// ── server.cfg ────────────────────────────────────────────────────────────────

struct ServerConfig
{
    // Server identity
    QString hostname;
    QString password;
    int svLan    = 0;   // sv_lan:    0 = public, 1 = LAN only
    int svRegion = 0;   // sv_region: -1 (world) .. 7 (Africa)

    // Gameplay — timing
    int mpTimelimit  = 0;   // mp_timelimit  (min; 0 = unlimited)
    int mpRoundtime  = 5;   // mp_roundtime  (min)
    int mpFreezetime = 6;   // mp_freezetime (sec)

    // Gameplay — toggles (1 = on, 0 = off)
    int mpFlashlight      = 1;
    int mpFootsteps       = 1;
    int mpFriendlyfire    = 0;
    int mpAutoteambalance = 1;
    int mpTkpunish        = 1;

    // Gameplay — limits
    int mpLimitteams     = 2;   // 0 = no limit
    int mpHostagepenalty = 5;   // 0 = disabled

    // Server behaviour
    int svMaxspeed = 320;
    int svCheats   = 0;
    int svAim      = 1;   // sv_aim: 1 = allow auto-aim, 0 = disabled
    int svPausable = 0;

    // Bots — quota/team (used by ServerPage)
    int     botQuota    = -1;   // -1 = not found in file
    QString botJoinTeam;

    // Bots — behaviour (used by BotsPage)
    QString botQuotaMode      = QStringLiteral("fill"); // fill | competitive
    int     botDifficulty     = 0;                      // 0‒3
    QString botChatter        = QStringLiteral("minimal"); // off|radio|minimal|normal
    int     botDeferToHuman   = 0;
    QString botPrefix         = {};
    int     botJoinAfterPlayer = 0;
    int     botAutoVacate     = 1;

    // Bots — allowed weapons (used by BotsPage)
    int botAllowPistols        = 1;
    int botAllowShotguns       = 1;
    int botAllowSubMachineGuns = 1;
    int botAllowRifles         = 1;
    int botAllowSnipers        = 1;
    int botAllowMachineGuns    = 1;
    int botAllowGrenades       = 1;
    int botAllowShield         = 0;
};

// Reads hostname and sv_password from server.cfg.
inline ServerConfig readServerConfig(const AppConfig::Game game)
{
    ServerConfig cfg;
    const QString path = gameDirectory(game) + QStringLiteral("/server.cfg");
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: could not open ") + path);
        return cfg;
    }

    auto parse = [&](const QString& line, const QString& key, auto& field)
    {
        const QString val = extractConfigValue(line, key);
        if (val.isEmpty()) return false;
        using T = std::remove_reference_t<decltype(field)>;
        if constexpr (std::is_same_v<T, QString>)
            field = val;
        else
            field = val.toInt();
        return true;
    };

    QTextStream stream(&f);
    while (stream.atEnd() == false)
    {
        const QString line = stream.readLine();

        // Identity
        if (parse(line, QStringLiteral("hostname"),     cfg.hostname))     continue;
        if (parse(line, QStringLiteral("sv_password"),  cfg.password))     continue;
        if (parse(line, QStringLiteral("sv_lan"),       cfg.svLan))        continue;
        if (parse(line, QStringLiteral("sv_region"),    cfg.svRegion))     continue;

        // Timing
        if (parse(line, QStringLiteral("mp_timelimit"),  cfg.mpTimelimit))  continue;
        if (parse(line, QStringLiteral("mp_roundtime"),  cfg.mpRoundtime))  continue;
        if (parse(line, QStringLiteral("mp_freezetime"), cfg.mpFreezetime)) continue;

        // Gameplay toggles
        if (parse(line, QStringLiteral("mp_flashlight"),      cfg.mpFlashlight))      continue;
        if (parse(line, QStringLiteral("mp_footsteps"),       cfg.mpFootsteps))       continue;
        if (parse(line, QStringLiteral("mp_friendlyfire"),    cfg.mpFriendlyfire))    continue;
        if (parse(line, QStringLiteral("mp_autoteambalance"), cfg.mpAutoteambalance)) continue;
        if (parse(line, QStringLiteral("mp_tkpunish"),        cfg.mpTkpunish))        continue;

        // Gameplay limits
        if (parse(line, QStringLiteral("mp_limitteams"),     cfg.mpLimitteams))     continue;
        if (parse(line, QStringLiteral("mp_hostagepenalty"), cfg.mpHostagepenalty)) continue;

        // Server behaviour
        if (parse(line, QStringLiteral("sv_maxspeed"), cfg.svMaxspeed)) continue;
        if (parse(line, QStringLiteral("sv_cheats"),   cfg.svCheats))   continue;
        if (parse(line, QStringLiteral("sv_aim"),      cfg.svAim))      continue;
        if (parse(line, QStringLiteral("sv_pausable"), cfg.svPausable)) continue;
        if (parse(line, QStringLiteral("pausable"),    cfg.svPausable)) continue; // alias

        // Bots — quota/team
        if (parse(line, QStringLiteral("bot_quota"),     cfg.botQuota))    continue;
        if (parse(line, QStringLiteral("bot_join_team"), cfg.botJoinTeam)) continue;

        // Bots — behaviour
        if (parse(line, QStringLiteral("bot_quota_mode"),       cfg.botQuotaMode))       continue;
        if (parse(line, QStringLiteral("bot_difficulty"),        cfg.botDifficulty))      continue;
        if (parse(line, QStringLiteral("bot_chatter"),           cfg.botChatter))         continue;
        if (parse(line, QStringLiteral("bot_defer_to_human"),    cfg.botDeferToHuman))    continue;
        if (parse(line, QStringLiteral("bot_prefix"),            cfg.botPrefix))          continue;
        if (parse(line, QStringLiteral("bot_join_after_player"), cfg.botJoinAfterPlayer)) continue;
        if (parse(line, QStringLiteral("bot_auto_vacate"),       cfg.botAutoVacate))      continue;

        // Bots — allowed weapons
        if (parse(line, QStringLiteral("bot_allow_pistols"),          cfg.botAllowPistols))        continue;
        if (parse(line, QStringLiteral("bot_allow_shotguns"),         cfg.botAllowShotguns))       continue;
        if (parse(line, QStringLiteral("bot_allow_sub_machine_guns"), cfg.botAllowSubMachineGuns)) continue;
        if (parse(line, QStringLiteral("bot_allow_rifles"),           cfg.botAllowRifles))         continue;
        if (parse(line, QStringLiteral("bot_allow_snipers"),          cfg.botAllowSnipers))        continue;
        if (parse(line, QStringLiteral("bot_allow_machine_guns"),     cfg.botAllowMachineGuns))    continue;
        if (parse(line, QStringLiteral("bot_allow_grenades"),         cfg.botAllowGrenades))       continue;
        if (parse(line, QStringLiteral("bot_allow_shield"),           cfg.botAllowShield))         continue;
    }

    DBG_APP(QStringLiteral("ServerFiles: read server.cfg — hostname=\"") + cfg.hostname
            + QStringLiteral("\" password=") + (cfg.password.isEmpty() ? QStringLiteral("(none)") : QStringLiteral("(set)")));
    return cfg;
}

// Updates (or appends) a single key in server.cfg, creating the file with
// defaults first if it does not yet exist.
inline bool writeServerConfigValue(const AppConfig::Game game,
                                   const QString& key,
                                   const QString& value)
{
    ensureServerConfig(game);

    const QString path = gameDirectory(game) + QStringLiteral("/server.cfg");

    if (QFile::exists(path) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: server.cfg not found at ") + path);
        return false;
    }

    QStringList lines;
    bool found = false;

    {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&f);
            while (in.atEnd() == false)
            {
                QString line = in.readLine();
                const QString trimmed = line.trimmed();

                const bool isComment = trimmed.startsWith(QStringLiteral("//"));
                const bool isTargetKey = (isComment == false)
                    && trimmed.startsWith(key)
                    && (trimmed.length() == key.length()
                        || trimmed[key.length()].isSpace());

                if (isTargetKey)
                {
                    line  = key + QStringLiteral(" \"") + value + u'"';
                    found = true;
                }
                lines.append(line);
            }
        }
    }

    if (found == false)
    {
        lines.append(key + QStringLiteral(" \"") + value + u'"');
    }

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: could not write ") + path);
        return false;
    }

    QTextStream out(&f);
    for (const QString& l : std::as_const(lines))
        out << l << u'\n';

    DBG_APP(QStringLiteral("ServerFiles: wrote ") + key + QStringLiteral("=\"") + value
            + QStringLiteral("\" to server.cfg"));
    return true;
}

// ── Map scanner ───────────────────────────────────────────────────────────────

// Returns a sorted, deduplicated list of map names from the game's maps/
// directory.  A name is included when either a .bsp or a .nav file with that
// base name exists — so maps bundled only as .nav (bot navmesh, .bsp inside a
// pak) and maps with both files each appear exactly once.
inline QStringList scanMaps(const AppConfig::Game game)
{
    const QString mapsPath = gameDirectory(game) + QStringLiteral("/maps");
    QDir dir(mapsPath);

    if (dir.exists() == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: maps directory not found: ") + mapsPath);
        return {};
    }

    dir.setNameFilters({QStringLiteral("*.bsp"), QStringLiteral("*.nav")});
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QSet<QString> nameSet;
    for (const QFileInfo& fi : dir.entryInfoList())
    {
        nameSet.insert(fi.completeBaseName());
    }

    QStringList names(nameSet.cbegin(), nameSet.cend());
    names.sort(Qt::CaseInsensitive);

    DBG_APP(QStringLiteral("ServerFiles: found ") + QString::number(names.size())
            + QStringLiteral(" unique maps in ") + mapsPath);
    return names;
}

} // namespace ServerFiles
