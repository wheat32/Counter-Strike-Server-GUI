#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

#include "appConfig.h"
#include "mapTypes.h"

class QPushButton;

class MapsPage : public QWidget
{
    Q_OBJECT

public:
    explicit MapsPage(QWidget* parent = nullptr);
    void loadForGame(AppConfig::Game game);

    // Update the dropdown to reflect a start map picked on the Server page,
    // turning on any filters that hide it. Does not emit mapSelected().
    void setStartMap(const QString& map);

signals:
    void mapSelected(const QString& map);
    void settingChanged();

private:
    void applyFilters();
    void openMapBrowser();

    // Turns on the version and type filters the map needs to be listed.
    void showInFilters(const QString& map);

    // Returns the filter checkbox for a map type.
    QCheckBox* typeFilter(MapTypes::MapType type) const;

    QComboBox*   m_mapCombo  = nullptr;
    QPushButton* m_browseBtn = nullptr;

    // Version filters
    QCheckBox* m_showStandard = nullptr;
    QCheckBox* m_showCZ       = nullptr;

    // Type filters
    QCheckBox* m_showDefuse        = nullptr;
    QCheckBox* m_showHostage       = nullptr;
    QCheckBox* m_showAssassination = nullptr;
    QCheckBox* m_showAim           = nullptr;
    QCheckBox* m_showFightYard     = nullptr;
    QCheckBox* m_showOther         = nullptr;

    QStringList m_allMaps;
};
