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

// Writes content to motd.txt. The game directory must already exist.
inline bool writeMotd(const AppConfig::Game game, const QString& content)
{
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
    QString hostname;
    QString password;
    int     timeLimit   = -1;   // mp_timelimit; -1 = not found in file
    int     botQuota    = -1;   // bot_quota;    -1 = not found in file
    QString botJoinTeam;        // bot_join_team: "T", "CT", "any", or empty
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

    QTextStream stream(&f);
    while (stream.atEnd() == false)
    {
        const QString line = stream.readLine();

        QString val = extractConfigValue(line, QStringLiteral("hostname"));
        if (val.isEmpty() == false) { cfg.hostname = val; continue; }

        val = extractConfigValue(line, QStringLiteral("sv_password"));
        if (val.isEmpty() == false) { cfg.password = val; continue; }

        val = extractConfigValue(line, QStringLiteral("mp_timelimit"));
        if (val.isEmpty() == false) { cfg.timeLimit = val.toInt(); continue; }

        val = extractConfigValue(line, QStringLiteral("bot_quota"));
        if (val.isEmpty() == false) { cfg.botQuota = val.toInt(); continue; }

        val = extractConfigValue(line, QStringLiteral("bot_join_team"));
        if (val.isEmpty() == false) { cfg.botJoinTeam = val; }
    }

    DBG_APP(QStringLiteral("ServerFiles: read server.cfg — hostname=\"") + cfg.hostname
            + QStringLiteral("\" password=") + (cfg.password.isEmpty() ? QStringLiteral("(none)") : QStringLiteral("(set)"))
            + QStringLiteral(" mp_timelimit=") + QString::number(cfg.timeLimit)
            + QStringLiteral(" bot_quota=") + QString::number(cfg.botQuota)
            + QStringLiteral(" bot_join_team=") + cfg.botJoinTeam);
    return cfg;
}

// Updates (or appends) a single key in server.cfg, preserving all other content.
// Does nothing and returns false if the file does not exist.
inline bool writeServerConfigValue(const AppConfig::Game game,
                                   const QString& key,
                                   const QString& value)
{
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
