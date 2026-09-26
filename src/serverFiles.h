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
// Values match the ServerConfig struct defaults so the UI and the file agree,
// except sv_minrate: new files raise it to the engine maximum (100000 bytes/s)
// for faster in-game downloads, while ServerConfig keeps the engine default (0)
// so a file without the key shows what the server actually uses.
inline QString defaultServerConfigContent()
{
    return QStringLiteral(
        "// CS Server Manager — generated default configuration\n"
        "\n"
        "hostname \"CS Server\"\n"
        "sv_password \"\"\n"
        "sv_lan 0\n"
        "sv_region 0\n"
        "sv_uploadmax 1\n"
        "sv_minrate 100000\n"
        "sv_maxrate 0\n"
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
    int svLan       = 0;   // sv_lan:       0 = public, 1 = LAN only
    int svRegion    = 0;   // sv_region:    -1 (world) .. 7 (Africa)
    int svUploadmax = 1;   // sv_uploadmax: largest file (MB) a client can upload
    int svMinrate   = 0;   // sv_minrate:   bytes/s floor on each player's rate (0 = none)
    int svMaxrate   = 0;   // sv_maxrate:   bytes/s cap on each player's rate (0 = none)

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

// Reads the settings the app manages from the server.cfg at path. Keys that are
// missing keep their ServerConfig defaults.
inline ServerConfig readServerConfigFile(const QString& path)
{
    ServerConfig cfg;
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
        if (parse(line, QStringLiteral("sv_uploadmax"), cfg.svUploadmax))  continue;
        if (parse(line, QStringLiteral("sv_minrate"),   cfg.svMinrate))    continue;
        if (parse(line, QStringLiteral("sv_maxrate"),   cfg.svMaxrate))    continue;

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

// Reads the settings the app manages from the given game's server.cfg.
inline ServerConfig readServerConfig(const AppConfig::Game game)
{
    return readServerConfigFile(gameDirectory(game) + QStringLiteral("/server.cfg"));
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

// The engine looks maps up as "maps/%.32s.bsp", so longer names can never load.
inline constexpr int MAX_MAP_NAME_LENGTH = 32;

// Returns the fallback_dir value from <gameDir>/liblist.gam (e.g. "cstrike" for
// czero), or an empty string if there is none. Mirrors the engine's parser: the
// line must start with the key and the value is the first quoted string.
inline QString readFallbackDir(const QString& gameDir)
{
    QFile f(gameDir + QStringLiteral("/liblist.gam"));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text) == false)
    {
        return QString();
    }

    const QString key = QStringLiteral("fallback_dir");
    QTextStream in(&f);
    while (in.atEnd() == false)
    {
        const QString line = in.readLine();
        if (line.startsWith(key, Qt::CaseInsensitive) == false) continue;

        const int open  = line.indexOf(u'"');
        const int close = (open < 0) ? open : line.indexOf(u'"', open + 1);
        if (close > open + 1)
        {
            return line.mid(open + 1, close - open - 1);
        }
    }
    return QString();
}

// Returns the directories whose maps/ folders the engine searches when a map is
// requested, in the engine's order: the game itself, <game>_downloads, then the
// liblist.gam fallback_dir. The engine also searches valve/ last, but that only
// holds Half-Life maps, so it is intentionally left out.
inline QStringList mapSearchDirectories(const QString& gameDir)
{
    const QFileInfo gameInfo(QDir::cleanPath(gameDir));
    const QString baseDir  = gameInfo.absolutePath();
    const QString gameName = gameInfo.fileName();

    QStringList dirs = {
        gameInfo.absoluteFilePath(),
        baseDir + u'/' + gameName + QStringLiteral("_downloads"),
    };

    const QString fallback = readFallbackDir(gameInfo.absoluteFilePath());
    const bool usableFallback = fallback.isEmpty() == false
        && fallback.compare(gameName, Qt::CaseInsensitive) != 0
        && fallback.compare(QStringLiteral("valve"), Qt::CaseInsensitive) != 0;
    if (usableFallback)
    {
        dirs.append(baseDir + u'/' + fallback);
    }
    return dirs;
}

// Returns a sorted, deduplicated list of the maps the server can load for the
// game content directory gameDir (e.g. ".../czero"). A map is loadable when
// maps/<name>.bsp exists in one of mapSearchDirectories(). Other files in maps/
// don't count: CZ ships .nav bot meshes for maps (e.g. awp_city) whose .bsp is
// not installed, and "map awp_city" fails with "not found on server".
inline QStringList scanMapsInGameDirectory(const QString& gameDir)
{
    QSet<QString> nameSet;

    for (const QString& searchDir : mapSearchDirectories(gameDir))
    {
        QDir dir(searchDir + QStringLiteral("/maps"));
        if (dir.exists() == false) continue;

        dir.setNameFilters({QStringLiteral("*.bsp")});
        dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

        for (const QFileInfo& fi : dir.entryInfoList())
        {
            const QString name = fi.completeBaseName();
            if (name.isEmpty() || name.length() > MAX_MAP_NAME_LENGTH) continue;
            nameSet.insert(name);
        }
    }

    QStringList names(nameSet.cbegin(), nameSet.cend());
    names.sort(Qt::CaseInsensitive);

    DBG_APP(QStringLiteral("ServerFiles: found ") + QString::number(names.size())
            + QStringLiteral(" loadable maps for ") + gameDir);
    return names;
}

// Returns the maps the server can load for the given game.
inline QStringList scanMaps(const AppConfig::Game game)
{
    return scanMapsInGameDirectory(gameDirectory(game));
}

// ── BotProfile.db ────────────────────────────────────────────────────────────

inline const QStringList& knownSkillTemplates()
{
    static const QStringList v = {
        QStringLiteral("Easy"), QStringLiteral("Fair"), QStringLiteral("Normal"),
        QStringLiteral("Tough"), QStringLiteral("Hard"), QStringLiteral("VeryHard"),
        QStringLiteral("Expert"), QStringLiteral("Elite")
    };
    return v;
}

inline const QStringList& knownWeaponTemplates()
{
    static const QStringList v = {
        QStringLiteral("Rifle"),   QStringLiteral("RifleT"),  QStringLiteral("Punch"),
        QStringLiteral("PunchT"),  QStringLiteral("Sniper"),  QStringLiteral("Power"),
        QStringLiteral("Shotgun"), QStringLiteral("Shield"),  QStringLiteral("Spray")
    };
    return v;
}

struct BotProfile
{
    QString name;
    QString skillTemplate;   // "Easy" .. "Elite"
    QString weaponTemplate;  // empty = none
    int     skin       = 0;  // 0 = any (unspecified)
    int     voicePitch = 100;
    int     cost       = -1; // -1 = unspecified (use template default)
};

inline QString botProfilePath(const AppConfig::Game game)
{
    return gameDirectory(game) + QStringLiteral("/BotProfile.db");
}

// Parses a bot profile header line such as "VeryHard+Sniper Quinn" or
// "Fair \"Kamala Harris\"". Returns false if the line is not a bot header.
inline bool parseBotHeader(const QString& trimmed,
                           QString& skillOut,
                           QString& weaponOut,
                           QString& nameOut)
{
    const int spaceIdx = trimmed.indexOf(u' ');
    if (spaceIdx < 0)
        return false;

    const QString templatePart = trimmed.left(spaceIdx);
    QString namePart = trimmed.mid(spaceIdx + 1).trimmed();

    // Strip inline comment
    const int commentIdx = namePart.indexOf(QStringLiteral("//"));
    if (commentIdx >= 0)
        namePart = namePart.left(commentIdx).trimmed();

    const int plusIdx = templatePart.indexOf(u'+');
    QString skill, weapon;
    if (plusIdx >= 0)
    {
        skill  = templatePart.left(plusIdx);
        weapon = templatePart.mid(plusIdx + 1);
    }
    else
    {
        skill = templatePart;
    }

    if (knownSkillTemplates().contains(skill) == false)
        return false;

    // Strip surrounding quotes from name
    if (namePart.startsWith(u'"') && namePart.endsWith(u'"') && namePart.length() >= 2)
        namePart = namePart.mid(1, namePart.length() - 2);

    if (namePart.isEmpty())
        return false;

    skillOut  = skill;
    weaponOut = weapon;
    nameOut   = namePart;
    return true;
}

// Reads the individual bot profiles from BotProfile.db.
// Template and Default blocks are skipped — only named bots are returned.
inline QVector<BotProfile> readBotProfiles(const AppConfig::Game game)
{
    QVector<BotProfile> result;
    QFile f(botProfilePath(game));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text) == false)
        return result;

    QTextStream in(&f);
    bool inBlock   = false; // inside a Template/Default block
    bool inProfile = false; // inside a named bot profile block
    BotProfile current;

    while (in.atEnd() == false)
    {
        const QString line    = in.readLine();
        const QString trimmed = line.trimmed();

        if (inBlock)
        {
            if (trimmed.compare(QStringLiteral("End"), Qt::CaseInsensitive) == 0)
                inBlock = false;
            continue;
        }

        if (inProfile)
        {
            if (trimmed.compare(QStringLiteral("End"), Qt::CaseInsensitive) == 0)
            {
                result.append(current);
                current    = {};
                inProfile  = false;
            }
            else if (trimmed.startsWith(QStringLiteral("Skin"), Qt::CaseInsensitive))
            {
                const int eq = trimmed.indexOf(u'=');
                if (eq >= 0) current.skin = trimmed.mid(eq + 1).trimmed().toInt();
            }
            else if (trimmed.startsWith(QStringLiteral("VoicePitch"), Qt::CaseInsensitive))
            {
                const int eq = trimmed.indexOf(u'=');
                if (eq >= 0) current.voicePitch = trimmed.mid(eq + 1).trimmed().toInt();
            }
            else if (trimmed.startsWith(QStringLiteral("Cost"), Qt::CaseInsensitive))
            {
                const int eq = trimmed.indexOf(u'=');
                if (eq >= 0) current.cost = trimmed.mid(eq + 1).trimmed().toInt();
            }
            continue;
        }

        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("//")))
            continue;

        if (trimmed.startsWith(QStringLiteral("Template"), Qt::CaseInsensitive) ||
            trimmed.startsWith(QStringLiteral("Default"), Qt::CaseInsensitive))
        {
            inBlock = true;
            continue;
        }

        QString skill, weapon, name;
        if (parseBotHeader(trimmed, skill, weapon, name))
        {
            current            = {};
            current.skillTemplate  = skill;
            current.weaponTemplate = weapon;
            current.name           = name;
            inProfile              = true;
        }
    }

    return result;
}

// Rewrites the individual bot profiles section of BotProfile.db, preserving
// the header (comments, Default block, Template blocks) verbatim.
inline bool writeBotProfiles(const AppConfig::Game game,
                             const QVector<BotProfile>& profiles)
{
    const QString path = botProfilePath(game);

    // ── Collect the header ────────────────────────────────────────────────────
    QStringList headerLines;
    {
        QFile rf(path);
        if (rf.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&rf);
            bool inBlock = false;
            QStringList pending;

            while (in.atEnd() == false)
            {
                const QString line    = in.readLine();
                const QString trimmed = line.trimmed();

                if (inBlock)
                {
                    pending.append(line);
                    if (trimmed.compare(QStringLiteral("End"), Qt::CaseInsensitive) == 0)
                    {
                        headerLines.append(pending);
                        pending.clear();
                        inBlock = false;
                    }
                    continue;
                }

                if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("//")))
                {
                    pending.append(line);
                    continue;
                }

                if (trimmed.startsWith(QStringLiteral("Template"), Qt::CaseInsensitive) ||
                    trimmed.startsWith(QStringLiteral("Default"), Qt::CaseInsensitive))
                {
                    headerLines.append(pending);
                    pending.clear();
                    pending.append(line);
                    inBlock = true;
                    continue;
                }

                // First individual bot profile — header ends here.
                headerLines.append(pending);
                break;
            }
        }
    }

    // ── Write ─────────────────────────────────────────────────────────────────
    if (QDir(gameDirectory(game)).exists() == false)
        return false;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    {
        DBG_APP(QStringLiteral("ServerFiles: could not write BotProfile.db"));
        return false;
    }

    QTextStream out(&f);

    for (const QString& l : std::as_const(headerLines))
        out << l << u'\n';

    out << QStringLiteral("\n//----------------------------------------------------------------------------\n\n");

    for (const BotProfile& bot : profiles)
    {
        QString header = bot.skillTemplate;
        if (bot.weaponTemplate.isEmpty() == false)
            header += u'+' + bot.weaponTemplate;

        if (bot.name.contains(u' '))
            header += QStringLiteral(" \"") + bot.name + u'"';
        else
            header += u' ' + bot.name;

        out << header << u'\n';

        if (bot.skin > 0)
            out << QStringLiteral("\tSkin = ") << bot.skin << u'\n';
        out << QStringLiteral("\tVoicePitch = ") << bot.voicePitch << u'\n';
        if (bot.cost >= 0)
            out << QStringLiteral("\tCost = ") << bot.cost << u'\n';

        out << QStringLiteral("End\n\n");
    }

    DBG_APP(QStringLiteral("ServerFiles: wrote BotProfile.db — ")
            + QString::number(profiles.size()) + QStringLiteral(" profiles"));
    return true;
}

} // namespace ServerFiles
