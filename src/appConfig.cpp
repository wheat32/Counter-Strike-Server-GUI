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

    DBG_SETTINGS(QStringLiteral("Config loaded from: ") + configFile());
    DBG_SETTINGS(QStringLiteral("  theme = ") + themeStr);
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

void AppConfig::resetToDefaults()
{
    DBG_SETTINGS(QStringLiteral("AppConfig::resetToDefaults()"));
    QFile::remove(configFile());
    m_theme = Theme::System;
}
