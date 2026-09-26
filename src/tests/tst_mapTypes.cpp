#include <QtTest/QtTest>

#include "mapTypes.h"

using MapTypes::MapType;

class TstMapTypes : public QObject
{
    Q_OBJECT

private slots:
    void detectType_knownPrefixes_matchType()
    {
        QCOMPARE(MapTypes::detectType(QStringLiteral("de_dust2")),     MapType::Defuse);
        QCOMPARE(MapTypes::detectType(QStringLiteral("cs_office_cz")), MapType::Hostage);
        QCOMPARE(MapTypes::detectType(QStringLiteral("as_oilrig")),    MapType::Assassination);
        QCOMPARE(MapTypes::detectType(QStringLiteral("aim_map")),      MapType::Aim);
        QCOMPARE(MapTypes::detectType(QStringLiteral("fy_iceworld")),  MapType::FightYard);
    }

    void detectType_mixedCase_matchesType()
    {
        QCOMPARE(MapTypes::detectType(QStringLiteral("DE_Nuke")), MapType::Defuse);
    }

    void detectType_unknownPrefix_isOther()
    {
        QCOMPARE(MapTypes::detectType(QStringLiteral("awp_city")), MapType::Other);
        QCOMPARE(MapTypes::detectType(QStringLiteral("dust")),     MapType::Other);
    }

    void isCZVariant_czSuffix_returnsTrue()
    {
        QVERIFY(MapTypes::isCZVariant(QStringLiteral("de_dust2_cz")));
        QVERIFY(MapTypes::isCZVariant(QStringLiteral("de_dust2")) == false);
    }

    void typeLabel_everyType_isNotEmpty()
    {
        for (const MapTypes::TypeInfo& info : MapTypes::typeInfos())
        {
            QVERIFY(MapTypes::typeLabel(info.type).isEmpty() == false);
        }
    }

    void groupByType_mixedMaps_displayOrderWithoutEmptyTypes()
    {
        const QStringList maps = {
            QStringLiteral("awp_city"), QStringLiteral("cs_italy"), QStringLiteral("de_aztec"),
            QStringLiteral("cs_747"),   QStringLiteral("de_dust"),
        };
        const QList<std::pair<MapType, QStringList>> groups = MapTypes::groupByType(maps);

        QCOMPARE(groups.size(), 3);
        QCOMPARE(groups.at(0).first, MapType::Hostage);
        QCOMPARE(groups.at(0).second, (QStringList{QStringLiteral("cs_italy"), QStringLiteral("cs_747")}));
        QCOMPARE(groups.at(1).first, MapType::Defuse);
        QCOMPARE(groups.at(1).second, (QStringList{QStringLiteral("de_aztec"), QStringLiteral("de_dust")}));
        QCOMPARE(groups.at(2).first, MapType::Other);
        QCOMPARE(groups.at(2).second, QStringList{QStringLiteral("awp_city")});
    }

    void typeInfos_prefixes_alphabeticalWithOtherLast()
    {
        const QList<MapTypes::TypeInfo>& infos = MapTypes::typeInfos();
        QCOMPARE(infos.last().type, MapType::Other);

        QStringList prefixes;
        for (const MapTypes::TypeInfo& info : infos)
        {
            if (info.type != MapType::Other)
            {
                prefixes.append(info.prefix);
            }
        }
        QStringList sorted = prefixes;
        sorted.sort();
        QCOMPARE(prefixes, sorted);
    }

    void groupByType_noMaps_returnsEmptyList()
    {
        QVERIFY(MapTypes::groupByType({}).isEmpty());
    }
};

QTEST_MAIN(TstMapTypes)
#include "tst_mapTypes.moc"
