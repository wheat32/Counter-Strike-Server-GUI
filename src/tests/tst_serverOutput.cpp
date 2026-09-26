#include <QtTest/QtTest>

#include "serverOutput.h"

// Lines below are real HLDS (czero, build 9907) console output.
class TstServerOutput : public QObject
{
    Q_OBJECT

private slots:
    void isServerReadyLine_vacStatusLines_returnTrue()
    {
        QVERIFY(ServerOutput::isServerReadyLine(QStringLiteral("   VAC secure mode is activated.")));
        QVERIFY(ServerOutput::isServerReadyLine(QStringLiteral("   VAC secure mode disabled.")));
        QVERIFY(ServerOutput::isServerReadyLine(QStringLiteral("   VAC secure mode not available.")));
    }

    void isServerReadyLine_otherLines_returnFalse()
    {
        QVERIFY(ServerOutput::isServerReadyLine(QStringLiteral("Server IP address 127.0.1.1:27990")) == false);
        QVERIFY(ServerOutput::isServerReadyLine(QStringLiteral("Navigation map loaded.")) == false);
    }

    void fatalErrorFromLine_portInUse_namesPort()
    {
        const QString reason = ServerOutput::fatalErrorFromLine(
            QStringLiteral("FATAL ERROR (shutting down): Couldn't allocate dedicated server IP port 27991."),
            true);
        QVERIFY(reason.contains(QStringLiteral("27991")));
        QVERIFY(reason.contains(QStringLiteral("already in use")));
    }

    void fatalErrorFromLine_portInUseAfterStartup_stillReported()
    {
        const QString line = QStringLiteral(
            "FATAL ERROR (shutting down): Couldn't allocate dedicated server IP port 27015.");
        QVERIFY(ServerOutput::fatalErrorFromLine(line, false).isEmpty() == false);
    }

    void fatalErrorFromLine_otherFatalError_returnsDetail()
    {
        QCOMPARE(ServerOutput::fatalErrorFromLine(
                     QStringLiteral("FATAL ERROR (shutting down): Could not load game DLL"), true),
                 QStringLiteral("Could not load game DLL"));
    }

    void fatalErrorFromLine_bindWarning_returnsEmpty()
    {
        // Printed just before the FATAL ERROR line — only the latter counts.
        QVERIFY(ServerOutput::fatalErrorFromLine(
                    QStringLiteral("WARNING: UDP_OpenSocket: port: 27991  bind: Address already in use"),
                    true).isEmpty());
    }

    void fatalErrorFromLine_mapNotFoundDuringStartup_namesMap()
    {
        const QString reason = ServerOutput::fatalErrorFromLine(
            QStringLiteral("map change failed: 'awp_city' not found on server."), true);
        QVERIFY(reason.contains(QStringLiteral("'awp_city'")));
    }

    void fatalErrorFromLine_mapNotFoundAfterStartup_returnsEmpty()
    {
        // A mistyped "map" command at the console leaves the current map running.
        QVERIFY(ServerOutput::fatalErrorFromLine(
                    QStringLiteral("map change failed: 'de_dsut2' not found on server."),
                    false).isEmpty());
    }

    void fatalErrorFromLine_hostErrorDuringStartup_returnsDetail()
    {
        QCOMPARE(ServerOutput::fatalErrorFromLine(
                     QStringLiteral("Host_Error: Mod_LoadBrushModel: maps/de_foo.bsp has wrong version number (20 should be 30)"),
                     true),
                 QStringLiteral("Mod_LoadBrushModel: maps/de_foo.bsp has wrong version number (20 should be 30)"));
    }

    void fatalErrorFromLine_hostErrorAfterStartup_returnsEmpty()
    {
        QVERIFY(ServerOutput::fatalErrorFromLine(
                    QStringLiteral("Host_Error: something"), false).isEmpty());
    }

    void fatalErrorFromLine_normalStartupLines_returnEmpty()
    {
        const QStringList lines = {
            QStringLiteral("Server IP address 127.0.1.1:27990"),
            QStringLiteral("couldn't exec maps/de_dust2_cz_load.cfg"),
            QStringLiteral("couldn't exec listip.cfg"),
            QStringLiteral("STEAM Auth Server"),
        };
        for (const QString& line : lines)
        {
            QVERIFY2(ServerOutput::fatalErrorFromLine(line, true).isEmpty(), qPrintable(line));
        }
    }
};

QTEST_MAIN(TstServerOutput)
#include "tst_serverOutput.moc"
