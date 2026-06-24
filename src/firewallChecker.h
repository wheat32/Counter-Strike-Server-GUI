#pragma once

#include <QObject>
#include <QTimer>

class QProcess;

// Checks whether a port is allowed by the system firewall (firewalld or ufw).
//
// firewalld path:
//   1. "systemctl is-active firewalld"  — no auth required
//   2. "firewall-cmd --list-all"        — one polkit prompt (replaces 3 separate ones)
//   3. Parse service XMLs from disk     — world-readable, no auth required
//
// ufw path:
//   1. "systemctl is-active ufw"        — no auth required
//   2. "ufw status"                     — one polkit prompt
class FirewallChecker : public QObject
{
    Q_OBJECT

    static constexpr int DEBOUNCE_MS = 600;

public:
    enum class Status       { Allowed, PartiallyAllowed, Blocked, Unknown };
    enum class FirewallType { Firewalld, UFW, Unknown };

    explicit FirewallChecker(QObject* parent = nullptr);

    void check(int port);
    void checkNow(int port);

signals:
    void resultReady(int port, FirewallChecker::Status status, FirewallChecker::FirewallType type);

private:
    QTimer*      m_debounce    = nullptr;
    int          m_pendingPort = 0;
    int          m_currentPort = 0;
    bool         m_tcpAllowed  = false;
    bool         m_udpAllowed  = false;
    FirewallType m_fwType      = FirewallType::Unknown;

    void startCheck();
    QProcess* launch(const QString& prog, const QStringList& args);

    void detectFirewalld();
    void checkFirewalldAll();
    void detectUfw();
    void checkUfwPorts();

    // Reads the service XML from disk and sets m_tcpAllowed/m_udpAllowed if
    // the target port appears in the service's <port> entries.
    void parseServiceXml(const QString& service, int port);

    // Returns true if targetPort matches a firewalld port spec ("27015" or "1714-1764").
    static bool portMatchesSpec(const QString& spec, int targetPort);

    void emitResult();
};
