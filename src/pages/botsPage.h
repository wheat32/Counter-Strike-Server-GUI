#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QWidget>

#include "appConfig.h"

class NumberSpinner;
class ToggleSwitch;
class ToggleWithStatus;

class BotsPage : public QWidget
{
    Q_OBJECT

public:
    explicit BotsPage(QWidget* parent = nullptr);
    void loadForGame(AppConfig::Game game);

signals:
    void settingChanged();

private:
    void save(const QString& key, const QString& value);
    void save(const QString& key, int value);
    void saveBotQuota();
    void saveBotTeam();

    // Quota & team (mirrors ServerPage quick-access controls)
    QWidget*          m_botOptions  = nullptr;  // count + team, shown when enabled
    ToggleWithStatus* m_botsEnabled = nullptr;
    NumberSpinner*    m_botCount    = nullptr;
    QCheckBox*        m_ctBots      = nullptr;
    QCheckBox*        m_tBots       = nullptr;

    // Behaviour
    QComboBox*    m_quotaMode        = nullptr;
    QComboBox*    m_difficulty        = nullptr;
    QComboBox*    m_chatter           = nullptr;
    QLineEdit*    m_prefix            = nullptr;
    ToggleSwitch* m_deferToHuman      = nullptr;
    ToggleSwitch* m_joinAfterPlayer   = nullptr;
    ToggleSwitch* m_autoVacate        = nullptr;

    // Allowed weapons
    ToggleSwitch* m_allowPistols        = nullptr;
    ToggleSwitch* m_allowShotguns       = nullptr;
    ToggleSwitch* m_allowSubMachineGuns = nullptr;
    ToggleSwitch* m_allowRifles         = nullptr;
    ToggleSwitch* m_allowSnipers        = nullptr;
    ToggleSwitch* m_allowMachineGuns    = nullptr;
    ToggleSwitch* m_allowGrenades       = nullptr;
    ToggleSwitch* m_allowShield         = nullptr;
};
