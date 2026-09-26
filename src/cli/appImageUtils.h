#pragma once
// appImageUtils.h
// Utilities for detecting and adapting to an AppImage runtime environment.

#include <QDir>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

// Returns true when this process is running inside an AppImage.
// The APPIMAGE environment variable is set by the AppImage runtime.
inline bool isRunningAsAppImage()
{
    return qEnvironmentVariableIsSet("APPIMAGE");
}

// Search-path variables that AppRun points at the AppImage's bundled libraries
// and Qt plugins.
inline const QStringList& appImageSearchPathVariables()
{
    static const QStringList vars = {
        QStringLiteral("LD_LIBRARY_PATH"),
        QStringLiteral("QT_PLUGIN_PATH"),
        QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH"),
    };
    return vars;
}

// Returns the environment for programs run on the host (hlds_run, systemctl,
// firewall-cmd, ...). Inside an AppImage, AppRun prepends the bundle's lib/ and
// plugins/ directories to the variables above. A host program that inherits them
// loads the bundled libraries instead of its own; firewall-cmd, for example,
// then dies with "GLIBC_2.44 not found". Every entry under $APPDIR is removed
// and a variable left empty is unset. Outside an AppImage the environment is
// returned unchanged.
inline QProcessEnvironment hostProcessEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (isRunningAsAppImage() == false || qEnvironmentVariableIsEmpty("APPDIR"))
    {
        return env;
    }

    const QString appDir = QDir::cleanPath(qEnvironmentVariable("APPDIR"));

    for (const QString& var : appImageSearchPathVariables())
    {
        if (env.contains(var) == false) continue;

        QStringList kept;
        for (const QString& entry : env.value(var).split(u':', Qt::SkipEmptyParts))
        {
            const bool inBundle = (entry == appDir) || entry.startsWith(appDir + u'/');
            if (inBundle == false)
            {
                kept.append(entry);
            }
        }

        if (kept.isEmpty())
        {
            env.remove(var);
        }
        else
        {
            env.insert(var, kept.join(u':'));
        }
    }
    return env;
}
