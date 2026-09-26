#pragma once

#include <QObject>
#include <QProcess>

// Manages a single hlds_run child process.
// All methods are safe to call from the main thread.
class ServerManager : public QObject
{
    Q_OBJECT

    static constexpr int QUIT_TIMEOUT_MS = 5000; // ms to wait for graceful quit

public:
    explicit ServerManager(QObject* parent = nullptr);

    [[nodiscard]] bool isRunning() const;

    // Launches hlds_run with the supplied parameters.  Returns false immediately
    // if the server is already running or the arguments are unusable; otherwise
    // returns true and startup errors are reported asynchronously via failed()
    // followed by stopped().
    bool start(const QString& serverPath,
               const QString& game,
               const QString& ip,
               int            port,
               const QString& map,
               int            maxPlayers);

    // Sends "quit" to stdin, then kills the process after QUIT_TIMEOUT_MS.
    void stop();

    // Writes a console command to the server's stdin.
    void sendCommand(const QString& cmd);

signals:
    // Emitted for every line of server output (stdout + stderr merged).
    void outputLine(const QString& line);

    // Emitted when the server hits an error that leaves it unusable, such as the
    // start map not existing or the port already being in use. The server is
    // already being stopped when this fires; stopped() follows.
    void failed(const QString& reason);

    // Emitted when the process exits for any reason (including error).
    void stopped();

private:
    QProcess* m_process       = nullptr;
    bool      m_startingUp    = false; // true until the first map is running
    bool      m_stopRequested = false; // stop() was called for this process
    bool      m_failed        = false; // failed() was already emitted for this process

    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void checkForFailure(const QString& line);
    void fail(const QString& reason);
};
