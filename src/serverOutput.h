#pragma once

#include <QCoreApplication>
#include <QRegularExpression>
#include <QString>

// Classifies lines of HLDS console output. Stateless — ServerManager tracks
// whether the server is still starting up and passes that in.
namespace ServerOutput
{

// HLDS prints one of these once the server is up and its first map is running:
//   "   VAC secure mode is activated."
//   "   VAC secure mode disabled."
//   "   VAC secure mode not available."
inline bool isServerReadyLine(const QString& line)
{
    return line.contains(QStringLiteral("VAC secure mode"));
}

// Returns a user-facing reason when line reports an error that leaves the
// server unusable, or an empty string otherwise.
//
// Map errors only count while startingUp: once a map is running, a failed
// "map" command typed at the console leaves that map running, but at startup
// the server is left idling with no map at all.
inline QString fatalErrorFromLine(const QString& line, const bool startingUp)
{
    // e.g. "FATAL ERROR (shutting down): Couldn't allocate dedicated server IP port 27015."
    // HLDS exits after printing this.
    static const QRegularExpression fatalRe(QStringLiteral("^FATAL ERROR[^:]*:\\s*(.*)$"));
    static const QRegularExpression portRe(
        QStringLiteral("Couldn't allocate dedicated server IP port (\\d+)"));

    const QString trimmed = line.trimmed();

    const QRegularExpressionMatch fatal = fatalRe.match(trimmed);
    if (fatal.hasMatch())
    {
        const QRegularExpressionMatch port = portRe.match(trimmed);
        if (port.hasMatch())
        {
            return QCoreApplication::translate("ServerOutput",
                "Port %1 is already in use. Another server or program may be using it.")
                .arg(port.captured(1));
        }
        return fatal.captured(1);
    }

    if (startingUp == false)
    {
        return QString();
    }

    // e.g. "map change failed: 'awp_city' not found on server."
    static const QRegularExpression mapRe(
        QStringLiteral("^map change failed: '([^']*)' not found on server"));
    const QRegularExpressionMatch map = mapRe.match(trimmed);
    if (map.hasMatch())
    {
        return QCoreApplication::translate("ServerOutput",
            "Map '%1' was not found on the server.").arg(map.captured(1));
    }

    // e.g. "Host_Error: Mod_LoadBrushModel: maps/de_foo.bsp has wrong version number"
    // aborts the map load and leaves the server with no map.
    static const QRegularExpression hostErrorRe(QStringLiteral("^Host_Error:\\s*(.*)$"));
    const QRegularExpressionMatch hostError = hostErrorRe.match(trimmed);
    if (hostError.hasMatch())
    {
        return hostError.captured(1);
    }

    return QString();
}

} // namespace ServerOutput
