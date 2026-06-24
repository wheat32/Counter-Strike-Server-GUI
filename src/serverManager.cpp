#include "serverManager.h"

#include <QTimer>

#include "cli/flatpakUtils.h"
#include "debug.h"

ServerManager::ServerManager(QObject* parent) : QObject(parent) {}

bool ServerManager::isRunning() const
{
    return m_process != nullptr && m_process->state() == QProcess::Running;
}

bool ServerManager::start(const QString& serverPath,
                          const QString& game,
                          const QString& ip,
                          const int      port,
                          const QString& map,
                          const int      maxPlayers)
{
    if (isRunning())
    {
        DBG_CLI(QStringLiteral("ServerManager::start() called while already running"));
        return false;
    }
    if (serverPath.isEmpty() || game.isEmpty())
    {
        DBG_CLI(QStringLiteral("ServerManager::start() — missing serverPath or game"));
        return false;
    }

    QStringList args;
    args << QStringLiteral("-game") << game;
    if (ip.isEmpty() == false)
    {
        args << QStringLiteral("-ip") << ip;
    }
    args << QStringLiteral("-port") << QString::number(port);
    if (map.isEmpty() == false)
    {
        args << QStringLiteral("+map") << map;
    }
    args << QStringLiteral("+maxplayers") << QString::number(maxPlayers);
    args << QStringLiteral("-console");

    const QString executable = serverPath + QStringLiteral("/hlds_run");

    DBG_CLI(QStringLiteral("ServerManager: launching: ") + executable
            + QStringLiteral(" ") + args.join(u' '));

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(serverPath);
    // Merge stderr into stdout so all output arrives on one channel.
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &ServerManager::onReadyRead);

    connect(m_process, &QProcess::finished,
            this, &ServerManager::onProcessFinished);

    connect(m_process, &QProcess::started, this, [this]()
    {
        DBG_CLI(QStringLiteral("ServerManager: hlds_run started (PID ")
                + QString::number(m_process->processId()) + QStringLiteral(")"));
        emit outputLine(QStringLiteral("=== Server started ==="));
    });

    connect(m_process, &QProcess::errorOccurred, this,
        [this](const QProcess::ProcessError error)
    {
        if (error == QProcess::FailedToStart)
        {
            const QString msg = m_process->errorString();
            DBG_CLI(QStringLiteral("ServerManager: failed to start: ") + msg);
            emit outputLine(QStringLiteral("[Error] Failed to start hlds_run: ") + msg
                            + QStringLiteral("\nVerify the server path in App Settings."));
            m_process->deleteLater();
            m_process = nullptr;
            emit stopped();
        }
    });

    // buildHostCommand wraps the call with flatpak-spawn --host when inside a
    // Flatpak sandbox and forwards the working directory via --directory=.
    auto [prog, fullArgs] = buildHostCommand(executable, args, serverPath);
    m_process->start(prog, fullArgs);

    return true; // result delivered asynchronously via stopped() on failure
}

void ServerManager::stop()
{
    if (m_process == nullptr) return;

    DBG_CLI(QStringLiteral("ServerManager: stopping server..."));
    emit outputLine(QStringLiteral("=== Stopping server ==="));

    sendCommand(QStringLiteral("quit"));
    m_process->closeWriteChannel();

    // Kill if the process hasn't exited after the grace period.
    QTimer::singleShot(QUIT_TIMEOUT_MS, m_process, [this]()
    {
        if (m_process != nullptr && m_process->state() == QProcess::Running)
        {
            DBG_CLI(QStringLiteral("ServerManager: kill after timeout"));
            m_process->kill();
        }
    });
}

void ServerManager::sendCommand(const QString& cmd)
{
    if (m_process == nullptr || m_process->state() != QProcess::Running) return;
    m_process->write((cmd + u'\n').toLocal8Bit());
}

void ServerManager::onReadyRead()
{
    const QString text = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    for (const QString& line : text.split(u'\n'))
    {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() == false)
        {
            emit outputLine(trimmed);
        }
    }
}

void ServerManager::onProcessFinished(const int exitCode, QProcess::ExitStatus)
{
    // Flush any buffered output that arrived just before exit.
    onReadyRead();

    DBG_CLI(QStringLiteral("ServerManager: process exited (code ")
            + QString::number(exitCode) + QStringLiteral(")"));
    emit outputLine(QStringLiteral("=== Server stopped (exit ") + QString::number(exitCode)
                    + QStringLiteral(") ==="));

    m_process->deleteLater();
    m_process = nullptr;
    emit stopped();
}
