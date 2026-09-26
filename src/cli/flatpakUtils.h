#pragma once
// flatpakUtils.h
// Shared utilities for detecting and adapting to a Flatpak sandbox environment.

#include "appImageUtils.h"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <utility>

// Returns true when this process is running inside a Flatpak sandbox.
// The FLATPAK_ID environment variable is always set by the Flatpak runtime.
inline bool isRunningAsFlatpak()
{
    return qEnvironmentVariableIsSet("FLATPAK_ID");
}

// Returns {program, fullArgs} to run an arbitrary host command via QProcess.
// - Inside Flatpak: forwards to the host via flatpak-spawn --host.
// - Otherwise (native or AppImage): returned unchanged; the system PATH is used.
//
// workingDir, if non-empty, sets the working directory of the spawned process.
// For native builds, also call QProcess::setWorkingDirectory() separately.
// For Flatpak, the directory is forwarded via --directory= to flatpak-spawn.
//
// Prefer startHostCommand(), which also handles the environment and working
// directory.
inline std::pair<QString, QStringList> buildHostCommand(const QString& program,
                                                        const QStringList& args = {},
                                                        const QString& workingDir = {})
{
    if (isRunningAsFlatpak())
    {
        QStringList spawnArgs;
        spawnArgs << QStringLiteral("--host");
        if (workingDir.isEmpty() == false)
        {
            spawnArgs << (QStringLiteral("--directory=") + workingDir);
        }
        spawnArgs << program << args;
        return {QStringLiteral("flatpak-spawn"), spawnArgs};
    }
    return {program, args};
}

// Starts a host program on proc. Use this instead of QProcess::start():
// - wraps the command with flatpak-spawn --host inside Flatpak (buildHostCommand)
// - removes the AppImage's bundled library paths from the environment
//   (hostProcessEnvironment)
// - sets the working directory when workingDir is non-empty
//
// Example:
//   startHostCommand(proc, "/path/hlds_run", {"-game", "czero"}, "/path");
inline void startHostCommand(QProcess*          proc,
                             const QString&     program,
                             const QStringList& args = {},
                             const QString&     workingDir = {})
{
    if (workingDir.isEmpty() == false)
    {
        proc->setWorkingDirectory(workingDir);
    }
    proc->setProcessEnvironment(hostProcessEnvironment());

    auto [prog, fullArgs] = buildHostCommand(program, args, workingDir);
    proc->start(prog, fullArgs);
}
