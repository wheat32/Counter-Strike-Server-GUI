#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <memory>

#include "serverFiles.h"

// Builds a fake HLDS install in a temporary directory:
//   <root>/czero/             (game, liblist.gam falls back to cstrike)
//   <root>/czero_downloads/
//   <root>/cstrike/
//   <root>/valve/
class TstServerFiles : public QObject
{
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_root;

    QString path(const QString& relative) const
    {
        return m_root->path() + u'/' + relative;
    }

    QString czeroDir() const
    {
        return path(QStringLiteral("czero"));
    }

    void writeFile(const QString& relative, const QByteArray& content = {})
    {
        const QString full = path(relative);
        QVERIFY(QDir().mkpath(QFileInfo(full).absolutePath()));
        QFile f(full);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(content);
    }

private slots:
    void init()
    {
        m_root = std::make_unique<QTemporaryDir>();
        QVERIFY(m_root->isValid());
        writeFile(QStringLiteral("czero/liblist.gam"),
                  "game \"Condition Zero\"\n"
                  "gamedll_linux \"dlls/cs.so\"\n"
                  "fallback_dir \"cstrike\"\n");
    }

    void cleanup()
    {
        m_root.reset();
    }

    void scanMapsInGameDirectory_bspInGameDir_isIncluded()
    {
        writeFile(QStringLiteral("czero/maps/de_dust2_cz.bsp"));
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir()),
                 QStringList{QStringLiteral("de_dust2_cz")});
    }

    void scanMapsInGameDirectory_navWithoutBsp_isExcluded()
    {
        // CZ ships awp_city.nav but not awp_city.bsp — the server can't load it.
        writeFile(QStringLiteral("czero/maps/awp_city.nav"));
        writeFile(QStringLiteral("czero/maps/de_aztec_cz.bsp"));
        writeFile(QStringLiteral("czero/maps/de_aztec_cz.nav"));
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir()),
                 QStringList{QStringLiteral("de_aztec_cz")});
    }

    void scanMapsInGameDirectory_partialDownload_isExcluded()
    {
        writeFile(QStringLiteral("czero/maps/de_gypt.bsp.ztmp"));
        QVERIFY(ServerFiles::scanMapsInGameDirectory(czeroDir()).isEmpty());
    }

    void scanMapsInGameDirectory_fallbackDirMaps_areIncluded()
    {
        writeFile(QStringLiteral("cstrike/maps/de_dust2.bsp"));
        writeFile(QStringLiteral("czero/maps/de_dust2_cz.bsp"));
        const QStringList expected = {QStringLiteral("de_dust2"), QStringLiteral("de_dust2_cz")};
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir()), expected);
    }

    void scanMapsInGameDirectory_noFallbackDir_ignoresOtherGames()
    {
        writeFile(QStringLiteral("czero/liblist.gam"), "game \"Condition Zero\"\n");
        writeFile(QStringLiteral("cstrike/maps/de_dust2.bsp"));
        QVERIFY(ServerFiles::scanMapsInGameDirectory(czeroDir()).isEmpty());
    }

    void scanMapsInGameDirectory_downloadsDirMaps_areIncluded()
    {
        writeFile(QStringLiteral("czero_downloads/maps/fy_pool_day.bsp"));
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir()),
                 QStringList{QStringLiteral("fy_pool_day")});
    }

    void scanMapsInGameDirectory_valveMaps_areExcluded()
    {
        writeFile(QStringLiteral("valve/maps/crossfire.bsp"));
        QVERIFY(ServerFiles::scanMapsInGameDirectory(czeroDir()).isEmpty());
    }

    void scanMapsInGameDirectory_duplicateAcrossDirs_listedOnce()
    {
        writeFile(QStringLiteral("czero/maps/de_nuke.bsp"));
        writeFile(QStringLiteral("cstrike/maps/de_nuke.bsp"));
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir()),
                 QStringList{QStringLiteral("de_nuke")});
    }

    void scanMapsInGameDirectory_nameLengths_onlyEngineLimitIncluded()
    {
        const QString longest(ServerFiles::MAX_MAP_NAME_LENGTH, u'a');
        const QString tooLong(ServerFiles::MAX_MAP_NAME_LENGTH + 1, u'b');
        writeFile(QStringLiteral("czero/maps/") + longest + QStringLiteral(".bsp"));
        writeFile(QStringLiteral("czero/maps/") + tooLong + QStringLiteral(".bsp"));
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir()), QStringList{longest});
    }

    void scanMapsInGameDirectory_trailingSlash_sameResult()
    {
        writeFile(QStringLiteral("cstrike/maps/de_dust2.bsp"));
        QCOMPARE(ServerFiles::scanMapsInGameDirectory(czeroDir() + u'/'),
                 QStringList{QStringLiteral("de_dust2")});
    }

    void scanMapsInGameDirectory_missingGameDir_returnsEmptyList()
    {
        QVERIFY(ServerFiles::scanMapsInGameDirectory(path(QStringLiteral("nope"))).isEmpty());
    }

    void readFallbackDir_quotedValue_returnsValue()
    {
        QCOMPARE(ServerFiles::readFallbackDir(czeroDir()), QStringLiteral("cstrike"));
    }

    void readFallbackDir_noKey_returnsEmpty()
    {
        writeFile(QStringLiteral("czero/liblist.gam"), "game \"Condition Zero\"\n");
        QVERIFY(ServerFiles::readFallbackDir(czeroDir()).isEmpty());
    }

    void readFallbackDir_noLiblist_returnsEmpty()
    {
        QVERIFY(ServerFiles::readFallbackDir(path(QStringLiteral("cstrike"))).isEmpty());
    }

    void readServerConfigFile_ratesMissing_useEngineDefaults()
    {
        writeFile(QStringLiteral("czero/server.cfg"), "hostname \"Test\"\n");
        const ServerFiles::ServerConfig cfg =
            ServerFiles::readServerConfigFile(path(QStringLiteral("czero/server.cfg")));
        QCOMPARE(cfg.svMinrate, 0);
        QCOMPARE(cfg.svMaxrate, 0);
    }

    void readServerConfigFile_ratesPresent_areRead()
    {
        writeFile(QStringLiteral("czero/server.cfg"),
                  "sv_minrate 100000 // fast downloads\n"
                  "sv_maxrate \"25000\"\n");
        const ServerFiles::ServerConfig cfg =
            ServerFiles::readServerConfigFile(path(QStringLiteral("czero/server.cfg")));
        QCOMPARE(cfg.svMinrate, 100000);
        QCOMPARE(cfg.svMaxrate, 25000);
    }

    void readServerConfigFile_defaultContent_minrateIsEngineMax()
    {
        writeFile(QStringLiteral("czero/server.cfg"),
                  ServerFiles::defaultServerConfigContent().toUtf8());
        const ServerFiles::ServerConfig cfg =
            ServerFiles::readServerConfigFile(path(QStringLiteral("czero/server.cfg")));
        QCOMPARE(cfg.svMinrate, 100000);
        QCOMPARE(cfg.svMaxrate, 0);
        QCOMPARE(cfg.svUploadmax, 1);
    }
};

QTEST_MAIN(TstServerFiles)
#include "tst_serverFiles.moc"
