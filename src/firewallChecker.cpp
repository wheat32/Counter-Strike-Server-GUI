#include "firewallChecker.h"

#include <QFile>
#include <QProcess>
#include <QXmlStreamReader>

#include "cli/flatpakUtils.h"
#include "debug.h"

FirewallChecker::FirewallChecker(QObject* parent) : QObject(parent)
{
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(DEBOUNCE_MS);
    connect(m_debounce, &QTimer::timeout, this, &FirewallChecker::startCheck);
}

void FirewallChecker::check(const int port)
{
    m_pendingPort = port;
    m_debounce->start();
}

void FirewallChecker::checkNow(const int port)
{
    m_debounce->stop();
    m_pendingPort = port;
    startCheck();
}

void FirewallChecker::startCheck()
{
    m_currentPort = m_pendingPort;
    m_tcpAllowed  = false;
    m_udpAllowed  = false;
    m_fwType      = FirewallType::Unknown;
    DBG_FW(QStringLiteral("Starting firewall check for port ") + QString::number(m_currentPort));
    detectFirewalld();
}

QProcess* FirewallChecker::launch(const QString& prog, const QStringList& args)
{
    auto [program, fullArgs] = buildHostCommand(prog, args);
    QProcess* proc = new QProcess(this);
    proc->start(program, fullArgs);
    return proc;
}

// ── firewalld ─────────────────────────────────────────────────────────────────

void FirewallChecker::detectFirewalld()
{
    const int port = m_currentPort;
    // systemctl is-active is a plain D-Bus read — no authentication needed.
    QProcess* proc = launch(QStringLiteral("systemctl"),
                            {QStringLiteral("is-active"), QStringLiteral("firewalld")});

    connect(proc, &QProcess::finished, this, [this, proc, port](int, QProcess::ExitStatus)
    {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        proc->deleteLater();

        if (port != m_currentPort) return;

        if (out == QLatin1String("active"))
        {
            DBG_FW(QStringLiteral("Firewall detected: firewalld"));
            m_fwType = FirewallType::Firewalld;
            checkFirewalldAll();
        }
        else
        {
            DBG_FW(QStringLiteral("firewalld not active — checking ufw"));
            detectUfw();
        }
    });
}

void FirewallChecker::checkFirewalldAll()
{
    const int port = m_currentPort;

    // ONE firewall-cmd call for the full zone picture — one polkit prompt instead
    // of the three we had before (--state, --query-port/tcp, --query-port/udp).
    QProcess* proc = launch(QStringLiteral("firewall-cmd"), {QStringLiteral("--list-all")});

    connect(proc, &QProcess::finished, this, [this, proc, port](int exitCode, QProcess::ExitStatus)
    {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        if (port != m_currentPort) return;

        if (exitCode != 0)
        {
            DBG_FW(QStringLiteral("firewall-cmd --list-all failed (exit ") + QString::number(exitCode) + QStringLiteral(")"));
            emit resultReady(m_currentPort, Status::Unknown, FirewallType::Firewalld);
            return;
        }

        const QString portTcp = QString::number(port) + QStringLiteral("/tcp");
        const QString portUdp = QString::number(port) + QStringLiteral("/udp");

        QStringList activeServices;

        for (const QString& line : out.split(u'\n'))
        {
            const QString trimmed = line.trimmed();

            if (trimmed.startsWith(QStringLiteral("ports:")))
            {
                const QString portList = trimmed.mid(6).trimmed();
                DBG_FW(QStringLiteral("Direct port rules: \"") + portList + QStringLiteral("\""));
                for (const QString& token : portList.split(u' ', Qt::SkipEmptyParts))
                {
                    if (token == portTcp)
                    {
                        DBG_FW(QStringLiteral("  Port ") + portTcp + QStringLiteral(" found via direct rule"));
                        m_tcpAllowed = true;
                    }
                    if (token == portUdp)
                    {
                        DBG_FW(QStringLiteral("  Port ") + portUdp + QStringLiteral(" found via direct rule"));
                        m_udpAllowed = true;
                    }
                }
            }
            else if (trimmed.startsWith(QStringLiteral("services:")))
            {
                const QString svcList = trimmed.mid(9).trimmed();
                DBG_FW(QStringLiteral("Active services: \"") + svcList + QStringLiteral("\""));
                if (svcList.isEmpty() == false)
                {
                    activeServices = svcList.split(u' ', Qt::SkipEmptyParts);
                }
            }
        }

        // Service XML files are world-readable — no additional auth needed.
        DBG_FW(QStringLiteral("Checking ") + QString::number(activeServices.size()) + QStringLiteral(" service XML files for port ") + QString::number(port));
        for (const QString& service : std::as_const(activeServices))
        {
            parseServiceXml(service, port);
        }

        emitResult();
    });
}

bool FirewallChecker::portMatchesSpec(const QString& spec, const int targetPort)
{
    if (spec.contains(u'-'))
    {
        // Port range, e.g. "1714-1764"
        const QStringList parts = spec.split(u'-');
        if (parts.size() == 2)
        {
            const int low  = parts[0].trimmed().toInt();
            const int high = parts[1].trimmed().toInt();
            return (targetPort >= low && targetPort <= high);
        }
        return false;
    }
    return (spec.trimmed().toInt() == targetPort);
}

void FirewallChecker::parseServiceXml(const QString& service, const int port)
{
    // User-defined services in /etc/firewalld/services/ override system defaults
    // in /usr/lib/firewalld/services/. Both are readable by any user.
    const QStringList searchPaths = {
        QStringLiteral("/etc/firewalld/services/") + service + QStringLiteral(".xml"),
        QStringLiteral("/usr/lib/firewalld/services/") + service + QStringLiteral(".xml"),
    };

    for (const QString& path : searchPaths)
    {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly) == false) continue;

        DBG_FW(QStringLiteral("  Parsing ") + path);

        QXmlStreamReader xml(&f);
        while (xml.atEnd() == false)
        {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QStringLiteral("port"))
            {
                const QString proto    = xml.attributes().value(QStringLiteral("protocol")).toString();
                const QString portSpec = xml.attributes().value(QStringLiteral("port")).toString();

                if (portMatchesSpec(portSpec, port))
                {
                    DBG_FW(QStringLiteral("  Port ") + QString::number(port) + QStringLiteral("/") + proto
                           + QStringLiteral(" found in service \"") + service + QStringLiteral("\" (spec: ") + portSpec + QStringLiteral(")"));
                    if (proto == QStringLiteral("tcp")) m_tcpAllowed = true;
                    if (proto == QStringLiteral("udp")) m_udpAllowed = true;
                }
            }
        }
        break; // found file in this location; don't fall through to the next
    }
}

// ── ufw ───────────────────────────────────────────────────────────────────────

void FirewallChecker::detectUfw()
{
    const int port = m_currentPort;
    QProcess* proc = launch(QStringLiteral("systemctl"),
                            {QStringLiteral("is-active"), QStringLiteral("ufw")});

    connect(proc, &QProcess::finished, this, [this, proc, port](int, QProcess::ExitStatus)
    {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        proc->deleteLater();

        if (port != m_currentPort) return;

        if (out == QLatin1String("active"))
        {
            DBG_FW(QStringLiteral("Firewall detected: ufw"));
            m_fwType = FirewallType::UFW;
            checkUfwPorts();
        }
        else
        {
            DBG_FW(QStringLiteral("ufw not active — no supported firewall detected"));
            emit resultReady(m_currentPort, Status::Unknown, FirewallType::Unknown);
        }
    });
}

void FirewallChecker::checkUfwPorts()
{
    const int port = m_currentPort;
    QProcess* proc = launch(QStringLiteral("ufw"), {QStringLiteral("status")});

    connect(proc, &QProcess::finished, this, [this, proc, port](int, QProcess::ExitStatus)
    {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        if (port != m_currentPort) return;

        if (out.contains(QStringLiteral("Status: active")) == false)
        {
            DBG_FW(QStringLiteral("ufw status: not active or could not read rules"));
            emit resultReady(m_currentPort, Status::Unknown, FirewallType::Unknown);
            return;
        }

        const QString tcpToken = QString::number(port) + QStringLiteral("/tcp");
        const QString udpToken = QString::number(port) + QStringLiteral("/udp");

        for (const QString& line : out.split(u'\n'))
        {
            if (line.contains(tcpToken) && line.contains(QStringLiteral("ALLOW")))
            {
                DBG_FW(QStringLiteral("Port ") + tcpToken + QStringLiteral(" found as ALLOW in ufw rules"));
                m_tcpAllowed = true;
            }
            if (line.contains(udpToken) && line.contains(QStringLiteral("ALLOW")))
            {
                DBG_FW(QStringLiteral("Port ") + udpToken + QStringLiteral(" found as ALLOW in ufw rules"));
                m_udpAllowed = true;
            }
        }

        emitResult();
    });
}

// ── result ────────────────────────────────────────────────────────────────────

void FirewallChecker::emitResult()
{
    Status status = Status::Unknown;
    if (m_tcpAllowed && m_udpAllowed)
    {
        status = Status::Allowed;
    }
    else if (m_tcpAllowed || m_udpAllowed)
    {
        status = Status::PartiallyAllowed;
    }
    else
    {
        status = Status::Blocked;
    }

    const QString statusStr = [status]() -> QString
    {
        switch (status)
        {
            case Status::Allowed:          return QStringLiteral("Allowed");
            case Status::PartiallyAllowed: return QStringLiteral("PartiallyAllowed");
            case Status::Blocked:          return QStringLiteral("Blocked");
            default:                       return QStringLiteral("Unknown");
        }
    }();
    const QString fwStr = (m_fwType == FirewallType::UFW)
        ? QStringLiteral("ufw")
        : QStringLiteral("firewalld");
    DBG_FW(QStringLiteral("Result for port ") + QString::number(m_currentPort)
           + QStringLiteral(": ") + statusStr
           + QStringLiteral(" (TCP=") + (m_tcpAllowed ? QStringLiteral("yes") : QStringLiteral("no"))
           + QStringLiteral(", UDP=") + (m_udpAllowed ? QStringLiteral("yes") : QStringLiteral("no"))
           + QStringLiteral(") via ") + fwStr);

    emit resultReady(m_currentPort, status, m_fwType);
}
