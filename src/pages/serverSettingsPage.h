#pragma once

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QWidget>

#include "appConfig.h"

class NumberSpinner;
class ToggleSwitch;

class ServerSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ServerSettingsPage(QWidget* parent = nullptr);
    void loadForGame(AppConfig::Game game);

signals:
    void settingChanged();

private:
    void save(const QString& key, const QString& value);
    void save(const QString& key, int value);

    // Server identity
    QLineEdit* m_hostname       = nullptr;
    QLineEdit* m_password       = nullptr;
    QAction*   m_passwordEyeAct = nullptr;
    ToggleSwitch* m_svLan       = nullptr;
    QComboBox*    m_svRegion    = nullptr;

    // Gameplay — timing
    NumberSpinner* m_mpTimelimit  = nullptr;
    NumberSpinner* m_mpRoundtime  = nullptr;
    NumberSpinner* m_mpFreezetime = nullptr;

    // Gameplay — toggles
    ToggleSwitch* m_mpFlashlight      = nullptr;
    ToggleSwitch* m_mpFootsteps       = nullptr;
    ToggleSwitch* m_mpFriendlyfire    = nullptr;
    ToggleSwitch* m_mpAutobalance     = nullptr;
    ToggleSwitch* m_mpTkpunish        = nullptr;

    // Gameplay — limits
    NumberSpinner* m_mpLimitteams     = nullptr;
    NumberSpinner* m_mpHostagepenalty = nullptr;

    // Advanced
    NumberSpinner* m_svMaxspeed = nullptr;
    ToggleSwitch*  m_svCheats   = nullptr;
    ToggleSwitch*  m_svAim      = nullptr;
    ToggleSwitch*  m_svPausable = nullptr;
};
