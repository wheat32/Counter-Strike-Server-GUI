#include <QtTest/QtTest>

#include "cli/appImageUtils.h"

class TstAppImageUtils : public QObject
{
    Q_OBJECT

    // Every variable the tests touch, saved in init() and restored in cleanup().
    const QStringList m_vars = {
        QStringLiteral("APPIMAGE"),
        QStringLiteral("APPDIR"),
        QStringLiteral("LD_LIBRARY_PATH"),
        QStringLiteral("QT_PLUGIN_PATH"),
        QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH"),
    };
    QHash<QString, QByteArray> m_saved;

    static void set(const char* name, const char* value)
    {
        qputenv(name, QByteArray(value));
    }

    static void enterAppImage()
    {
        set("APPIMAGE", "/home/user/CSServerManager.AppImage");
        set("APPDIR",   "/tmp/.mount_CSSabc");
    }

private slots:
    void init()
    {
        m_saved.clear();
        for (const QString& var : m_vars)
        {
            const QByteArray name = var.toLatin1();
            if (qEnvironmentVariableIsSet(name.constData()))
            {
                m_saved.insert(var, qgetenv(name.constData()));
            }
            qunsetenv(name.constData());
        }
    }

    void cleanup()
    {
        for (const QString& var : m_vars)
        {
            const QByteArray name = var.toLatin1();
            if (m_saved.contains(var))
            {
                qputenv(name.constData(), m_saved.value(var));
            }
            else
            {
                qunsetenv(name.constData());
            }
        }
    }

    void hostProcessEnvironment_notAppImage_leavesPathsAlone()
    {
        set("APPDIR", "/tmp/.mount_CSSabc");
        set("LD_LIBRARY_PATH", "/tmp/.mount_CSSabc/usr/lib");
        const QProcessEnvironment env = hostProcessEnvironment();
        QCOMPARE(env.value(QStringLiteral("LD_LIBRARY_PATH")), QStringLiteral("/tmp/.mount_CSSabc/usr/lib"));
    }

    void hostProcessEnvironment_appImage_stripsBundleEntries()
    {
        enterAppImage();
        // AppRun leaves a trailing ':' when LD_LIBRARY_PATH was unset.
        set("LD_LIBRARY_PATH", "/tmp/.mount_CSSabc/usr/lib:/opt/steam/lib:");
        set("QT_PLUGIN_PATH", "/tmp/.mount_CSSabc/usr/plugins:/usr/lib/qt6/plugins");
        const QProcessEnvironment env = hostProcessEnvironment();
        QCOMPARE(env.value(QStringLiteral("LD_LIBRARY_PATH")), QStringLiteral("/opt/steam/lib"));
        QCOMPARE(env.value(QStringLiteral("QT_PLUGIN_PATH")), QStringLiteral("/usr/lib/qt6/plugins"));
    }

    void hostProcessEnvironment_appImage_unsetsEmptiedVariables()
    {
        enterAppImage();
        set("LD_LIBRARY_PATH", "/tmp/.mount_CSSabc/usr/lib:");
        set("QT_QPA_PLATFORM_PLUGIN_PATH", "/tmp/.mount_CSSabc/usr/plugins/platforms");
        const QProcessEnvironment env = hostProcessEnvironment();
        QVERIFY(env.contains(QStringLiteral("LD_LIBRARY_PATH")) == false);
        QVERIFY(env.contains(QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH")) == false);
    }

    void hostProcessEnvironment_appImage_keepsSimilarPrefix()
    {
        enterAppImage();
        set("LD_LIBRARY_PATH", "/tmp/.mount_CSSabcdef/usr/lib");
        const QProcessEnvironment env = hostProcessEnvironment();
        QCOMPARE(env.value(QStringLiteral("LD_LIBRARY_PATH")), QStringLiteral("/tmp/.mount_CSSabcdef/usr/lib"));
    }

    void hostProcessEnvironment_appImage_keepsOtherVariables()
    {
        enterAppImage();
        const QProcessEnvironment env = hostProcessEnvironment();
        QCOMPARE(env.value(QStringLiteral("APPDIR")), QStringLiteral("/tmp/.mount_CSSabc"));
    }
};

QTEST_MAIN(TstAppImageUtils)
#include "tst_appImageUtils.moc"
