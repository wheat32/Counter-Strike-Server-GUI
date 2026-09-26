#pragma once

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <utility>

// Classifies CS maps by the prefix of their name (de_, cs_, ...).
namespace MapTypes
{

enum class MapType { Defuse, Hostage, Assassination, Aim, FightYard, Other };

struct TypeInfo
{
    MapType     type;
    QString     prefix; // empty for Other
    const char* label;  // untranslated; use typeLabel()
};

// Every map type, in display order: alphabetical by prefix, with Other (which
// matches anything) last. Keep new prefixes in alphabetical position.
inline const QList<TypeInfo>& typeInfos()
{
    static const QList<TypeInfo> infos = {
        {MapType::Aim,           QStringLiteral("aim_"), QT_TRANSLATE_NOOP("MapTypes", "Aim training (aim_)")},
        {MapType::Assassination, QStringLiteral("as_"),  QT_TRANSLATE_NOOP("MapTypes", "Assassination (as_)")},
        {MapType::Hostage,       QStringLiteral("cs_"),  QT_TRANSLATE_NOOP("MapTypes", "Hostage rescue (cs_)")},
        {MapType::Defuse,        QStringLiteral("de_"),  QT_TRANSLATE_NOOP("MapTypes", "Defuse (de_)")},
        {MapType::FightYard,     QStringLiteral("fy_"),  QT_TRANSLATE_NOOP("MapTypes", "Fight Yard (fy_)")},
        {MapType::Other,         QString(),              QT_TRANSLATE_NOOP("MapTypes", "Other")},
    };
    return infos;
}

inline MapType detectType(const QString& name)
{
    for (const TypeInfo& info : typeInfos())
    {
        if (info.prefix.isEmpty() == false && name.startsWith(info.prefix, Qt::CaseInsensitive))
        {
            return info.type;
        }
    }
    return MapType::Other;
}

// Returns the display label for a type, e.g. "Defuse (de_)".
inline QString typeLabel(const MapType type)
{
    for (const TypeInfo& info : typeInfos())
    {
        if (info.type == type)
        {
            return QCoreApplication::translate("MapTypes", info.label);
        }
    }
    return QString();
}

// True for Condition Zero remakes of standard maps (e.g. de_dust2_cz).
inline bool isCZVariant(const QString& name)
{
    return name.endsWith(QStringLiteral("_cz"), Qt::CaseInsensitive);
}

// Groups maps by type in display order, keeping each group's input order.
// Types without any maps are left out.
inline QList<std::pair<MapType, QStringList>> groupByType(const QStringList& maps)
{
    QList<std::pair<MapType, QStringList>> groups;
    for (const TypeInfo& info : typeInfos())
    {
        QStringList members;
        for (const QString& map : maps)
        {
            if (detectType(map) == info.type)
            {
                members.append(map);
            }
        }
        if (members.isEmpty() == false)
        {
            groups.append({info.type, members});
        }
    }
    return groups;
}

} // namespace MapTypes
