#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

#include "appConfig.h"

class MapsPage : public QWidget
{
    Q_OBJECT

public:
    explicit MapsPage(QWidget* parent = nullptr);
    void loadForGame(AppConfig::Game game);

signals:
    void mapSelected(const QString& map);
    void settingChanged();

private:
    void applyFilters();

    QComboBox* m_mapCombo = nullptr;

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
