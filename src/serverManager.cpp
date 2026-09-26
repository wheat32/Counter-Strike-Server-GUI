#include "serverManager.h"

#include <QTimer>

#include "cli/flatpakUtils.h"
#include "debug.h"
#include "serverOutput.h"

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
    // hlds_run otherwise restarts hlds_linux forever, even after "quit" or a
    // fatal startup error, so the server would never stop on its own. With
    // -norestart it exec()s hlds_linux, so this process is the server itself.
    args << QStringLiteral("-norestart");

    const QString executable = serverPath + QStringLiteral("/hlds_run");

    DBG_CLI(QStringLiteral("ServerManager: launching: ") + executable
            + QStringLiteral(" ") + args.join(u' '));

    m_process       = new QProcess(this);
    m_startingUp    = (map.isEmpty() == false);
    m_stopRequested = false;
    m_failed        = false;
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
            emit failed(tr("hlds_run could not be started: %1").arg(msg));
            emit stopped();
        }
    });

    startHostCommand(m_process, executable, args, serverPath);

    return true; // failures are reported asynchronously via failed() and stopped()
}

void ServerManager::stop()
{
    if (m_process == nullptr) return;

    DBG_CLI(QStringLiteral("ServerManager: stopping server..."));
    emit outputLine(QStringLiteral("=== Stopping server ==="));
    m_stopRequested = true;

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
            checkForFailure(trimmed);
        }
    }
}

void ServerManager::checkForFailure(const QString& line)
{
    if (m_startingUp && ServerOutput::isServerReadyLine(line))
    {
        DBG_CLI(QStringLiteral("ServerManager: server is up"));
        m_startingUp = false;
        return;
    }

    const QString reason = ServerOutput::fatalErrorFromLine(line, m_startingUp);
    if (reason.isEmpty() == false)
    {
        fail(reason);
    }
}

void ServerManager::fail(const QString& reason)
{
    if (m_failed) return;
    m_failed = true;

    DBG_CLI(QStringLiteral("ServerManager: server failed: ") + reason);
    // Stop before emitting: a receiver of failed() may open a dialog.
    if (isRunning() && m_stopRequested == false)
    {
        stop();
    }
    emit failed(reason);
}

void ServerManager::onProcessFinished(const int exitCode, QProcess::ExitStatus)
{
    // Flush any buffered output that arrived just before exit.
    onReadyRead();

    DBG_CLI(QStringLiteral("ServerManager: process exited (code ")
            + QString::number(exitCode) + QStringLiteral(")"));
    emit outputLine(QStringLiteral("=== Server stopped (exit ") + QString::number(exitCode)
                    + QStringLiteral(") ==="));

    // Exited during startup without a recognized error line, e.g. hlds_run
    // rejecting its arguments.
    if (m_startingUp && m_stopRequested == false)
    {
        fail(tr("The server exited during startup (exit code %1). "
                "See Server Controls for the console output.").arg(exitCode));
    }
    m_startingUp = false;

    m_process->deleteLater();
    m_process = nullptr;
    emit stopped();
}
