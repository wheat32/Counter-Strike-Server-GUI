#include "botsPage.h"

#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "appConfig.h"
#include "debug.h"
#include "serverFiles.h"
#include "widgets/numberSpinner.h"
#include "widgets/toggleSwitch.h"
#include "widgets/toggleWithStatus.h"

namespace
{
constexpr int PAGE_MARGIN     = 20;
constexpr int GROUP_SPACING   = 16;
constexpr int ROW_SPACING     = 10;
constexpr int CTRL_SPACING    = 12;
constexpr int HINT_SPACING    = 2;
constexpr int COMBO_MIN_WIDTH = 140;
constexpr int BOT_COUNT_MIN   = 0;
constexpr int BOT_COUNT_MAX   = 32;
constexpr int BOT_TEAM_SPACING = 20;
} // namespace

BotsPage::BotsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel(tr("Bots"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    outerLayout->addWidget(titleLabel);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scroll, 1);

    QWidget* content = new QWidget(scroll);
    scroll->setWidget(content);

    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN);
    contentLayout->setSpacing(GROUP_SPACING);

    // ── Row helpers ───────────────────────────────────────────────────────────

    auto addToggleRow = [&](QVBoxLayout* grp, const QString& label,
                            const QString& hint = {}) -> ToggleSwitch*
    {
        QHBoxLayout* row = new QHBoxLayout;
        row->setSpacing(CTRL_SPACING);

        QVBoxLayout* labelCol = new QVBoxLayout;
        labelCol->setSpacing(HINT_SPACING);
        labelCol->setContentsMargins(0, 0, 0, 0);
        labelCol->addWidget(new QLabel(label, content));
        if (hint.isEmpty() == false)
        {
            QLabel* h = new QLabel(hint, content);
            h->setStyleSheet(QStringLiteral("color: #888;"));
            labelCol->addWidget(h);
        }
        row->addLayout(labelCol, 1);

        ToggleSwitch* toggle = new ToggleSwitch(content);
        row->addWidget(toggle, 0, Qt::AlignVCenter);
        grp->addLayout(row);
        return toggle;
    };

    auto addComboRow = [&](QVBoxLayout* grp, const QString& label,
                           const QString& hint = {}) -> QComboBox*
    {
        QHBoxLayout* row = new QHBoxLayout;
        row->setSpacing(CTRL_SPACING);

        QVBoxLayout* labelCol = new QVBoxLayout;
        labelCol->setSpacing(HINT_SPACING);
        labelCol->setContentsMargins(0, 0, 0, 0);
        labelCol->addWidget(new QLabel(label, content));
        if (hint.isEmpty() == false)
        {
            QLabel* h = new QLabel(hint, content);
            h->setStyleSheet(QStringLiteral("color: #888;"));
            labelCol->addWidget(h);
        }
        row->addLayout(labelCol, 1);

        QComboBox* combo = new QComboBox(content);
        combo->setMinimumWidth(COMBO_MIN_WIDTH);
        row->addWidget(combo, 0, Qt::AlignVCenter);
        grp->addLayout(row);
        return combo;
    };

    // ── Quota & Team ──────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Quota & Team"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(ROW_SPACING);

        // Enable row
        QHBoxLayout* enableRow = new QHBoxLayout;
        enableRow->setSpacing(CTRL_SPACING);
        QVBoxLayout* enableLabel = new QVBoxLayout;
        enableLabel->setSpacing(HINT_SPACING);
        enableLabel->setContentsMargins(0, 0, 0, 0);
        enableLabel->addWidget(new QLabel(tr("Enable bots"), content));
        QLabel* enableHint = new QLabel(tr("Add AI players to fill empty slots (bot_quota)"), content);
        enableHint->setStyleSheet(QStringLiteral("color: #888;"));
        enableLabel->addWidget(enableHint);
        enableRow->addLayout(enableLabel, 1);
        m_botsEnabled = new ToggleWithStatus(content);
        enableRow->addWidget(m_botsEnabled, 0, Qt::AlignVCenter);
        grp->addLayout(enableRow);

        // Options shown only when bots are enabled
        m_botOptions = new QWidget(group);
        QVBoxLayout* optLayout = new QVBoxLayout(m_botOptions);
        optLayout->setContentsMargins(0, 0, 0, 0);
        optLayout->setSpacing(ROW_SPACING);

        // Count row
        QHBoxLayout* countRow = new QHBoxLayout;
        countRow->setSpacing(CTRL_SPACING);
        countRow->addWidget(new QLabel(tr("Bot count"), m_botOptions), 1);
        m_botCount = new NumberSpinner(m_botOptions);
        m_botCount->setRange(BOT_COUNT_MIN, BOT_COUNT_MAX);
        m_botCount->setValue(5);
        countRow->addWidget(m_botCount, 0, Qt::AlignVCenter);
        optLayout->addLayout(countRow);

        // Team row
        QHBoxLayout* teamRow = new QHBoxLayout;
        teamRow->setSpacing(CTRL_SPACING);
        teamRow->addWidget(new QLabel(tr("Bot teams"), m_botOptions), 1);
        QWidget* teamBox = new QWidget(m_botOptions);
        QHBoxLayout* teamLayout = new QHBoxLayout(teamBox);
        teamLayout->setContentsMargins(0, 0, 0, 0);
        teamLayout->setSpacing(BOT_TEAM_SPACING);
        m_ctBots = new QCheckBox(tr("Counter-Terrorist"), teamBox);
        m_tBots  = new QCheckBox(tr("Terrorist"), teamBox);
        m_ctBots->setChecked(true);
        m_tBots->setChecked(true);
        teamLayout->addWidget(m_ctBots);
        teamLayout->addWidget(m_tBots);
        teamLayout->addStretch();
        teamRow->addWidget(teamBox);
        optLayout->addLayout(teamRow);

        grp->addWidget(m_botOptions);
        m_botOptions->setVisible(false);
        contentLayout->addWidget(group);

        connect(m_botsEnabled, &ToggleWithStatus::toggled, m_botOptions, &QWidget::setVisible);
        connect(m_botsEnabled, &ToggleWithStatus::toggled, this, [this](bool) { saveBotQuota(); });
        connect(m_botCount, &NumberSpinner::valueChanged, this, [this](int) { saveBotQuota(); });
        connect(m_ctBots, &QCheckBox::toggled, this, [this](bool) { saveBotTeam(); });
        connect(m_tBots,  &QCheckBox::toggled, this, [this](bool) { saveBotTeam(); });
    }

    // ── Behaviour ─────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Behaviour"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(ROW_SPACING);

        m_difficulty = addComboRow(grp, tr("Difficulty"),
                                   tr("Skill level of the bot AI"));
        m_difficulty->addItem(tr("Easiest"), 0);
        m_difficulty->addItem(tr("Easy"),    1);
        m_difficulty->addItem(tr("Normal"),  2);
        m_difficulty->addItem(tr("Hard"),    3);

        m_chatter = addComboRow(grp, tr("Chatter"),
                                tr("How much bots communicate over voice radio"));
        m_chatter->addItem(tr("Off"),     QStringLiteral("off"));
        m_chatter->addItem(tr("Radio"),   QStringLiteral("radio"));
        m_chatter->addItem(tr("Minimal"), QStringLiteral("minimal"));
        m_chatter->addItem(tr("Normal"),  QStringLiteral("normal"));

        m_quotaMode = addComboRow(grp, tr("Quota mode"),
                                  tr("How the server fills slots with bots"));
        m_quotaMode->addItem(tr("Fill"),        QStringLiteral("fill"));
        m_quotaMode->addItem(tr("Competitive"), QStringLiteral("competitive"));

        // Name prefix — full-width text input
        grp->addWidget(new QLabel(tr("Name prefix:"), group));
        m_prefix = new QLineEdit(group);
        m_prefix->setPlaceholderText(tr("e.g. [BOT] — leave empty for none"));
        grp->addWidget(m_prefix);

        m_deferToHuman = addToggleRow(grp, tr("Defer to human players"),
                                      tr("Bots wait for a human player to act first"));

        m_joinAfterPlayer = addToggleRow(grp, tr("Wait for human before joining"),
                                         tr("Bots only join once a human player is present"));

        m_autoVacate = addToggleRow(grp, tr("Auto-vacate for humans"),
                                    tr("Bots leave to make room when humans join"));

        contentLayout->addWidget(group);

        connect(m_difficulty, &QComboBox::currentIndexChanged, this, [this]()
        {
            save(QStringLiteral("bot_difficulty"), m_difficulty->currentData().toInt());
        });
        connect(m_chatter, &QComboBox::currentIndexChanged, this, [this]()
        {
            save(QStringLiteral("bot_chatter"), m_chatter->currentData().toString());
        });
        connect(m_quotaMode, &QComboBox::currentIndexChanged, this, [this]()
        {
            save(QStringLiteral("bot_quota_mode"), m_quotaMode->currentData().toString());
        });
        connect(m_prefix, &QLineEdit::editingFinished, this, [this]()
        {
            save(QStringLiteral("bot_prefix"), m_prefix->text().trimmed());
        });
        connect(m_deferToHuman, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_defer_to_human"), on ? 1 : 0);
        });
        connect(m_joinAfterPlayer, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_join_after_player"), on ? 1 : 0);
        });
        connect(m_autoVacate, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_auto_vacate"), on ? 1 : 0);
        });
    }

    // ── Allowed Weapons ───────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Allowed Weapons"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(ROW_SPACING);

        m_allowPistols = addToggleRow(grp, tr("Pistols"),
                                      tr("bot_allow_pistols"));
        m_allowShotguns = addToggleRow(grp, tr("Shotguns"),
                                       tr("bot_allow_shotguns"));
        m_allowSubMachineGuns = addToggleRow(grp, tr("Sub-machine guns"),
                                             tr("bot_allow_sub_machine_guns"));
        m_allowRifles = addToggleRow(grp, tr("Rifles"),
                                     tr("bot_allow_rifles"));
        m_allowSnipers = addToggleRow(grp, tr("Sniper rifles"),
                                      tr("bot_allow_snipers"));
        m_allowMachineGuns = addToggleRow(grp, tr("Machine guns"),
                                          tr("bot_allow_machine_guns"));
        m_allowGrenades = addToggleRow(grp, tr("Grenades"),
                                       tr("bot_allow_grenades"));
        m_allowShield = addToggleRow(grp, tr("Tactical shield"),
                                     tr("bot_allow_shield"));

        contentLayout->addWidget(group);

        connect(m_allowPistols, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_pistols"), on ? 1 : 0);
        });
        connect(m_allowShotguns, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_shotguns"), on ? 1 : 0);
        });
        connect(m_allowSubMachineGuns, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_sub_machine_guns"), on ? 1 : 0);
        });
        connect(m_allowRifles, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_rifles"), on ? 1 : 0);
        });
        connect(m_allowSnipers, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_snipers"), on ? 1 : 0);
        });
        connect(m_allowMachineGuns, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_machine_guns"), on ? 1 : 0);
        });
        connect(m_allowGrenades, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_grenades"), on ? 1 : 0);
        });
        connect(m_allowShield, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("bot_allow_shield"), on ? 1 : 0);
        });
    }

    contentLayout->addStretch();

    loadForGame(AppConfig::instance().selectedGame());
}

void BotsPage::loadForGame(const AppConfig::Game game)
{
    const ServerFiles::ServerConfig cfg = ServerFiles::readServerConfig(game);

    auto block = [](QObject* obj, bool b) { obj->blockSignals(b); };

    // Block all signals during load
    block(m_botsEnabled,        true);
    block(m_botCount,           true);
    block(m_ctBots,             true);
    block(m_tBots,              true);
    block(m_quotaMode,          true);
    block(m_difficulty,         true);
    block(m_chatter,            true);
    block(m_prefix,             true);
    block(m_deferToHuman,       true);
    block(m_joinAfterPlayer,    true);
    block(m_autoVacate,         true);
    block(m_allowPistols,       true);
    block(m_allowShotguns,      true);
    block(m_allowSubMachineGuns,true);
    block(m_allowRifles,        true);
    block(m_allowSnipers,       true);
    block(m_allowMachineGuns,   true);
    block(m_allowGrenades,      true);
    block(m_allowShield,        true);

    // Quota & team
    const bool botsOn = (cfg.botQuota > 0);
    m_botsEnabled->setOn(botsOn, false);
    m_botOptions->setVisible(botsOn);
    if (cfg.botQuota > 0)
        m_botCount->setValue(cfg.botQuota);

    {
        const QString team = cfg.botJoinTeam.toLower();
        m_ctBots->setChecked(team.isEmpty() || team == QStringLiteral("ct") || team == QStringLiteral("any"));
        m_tBots->setChecked(team.isEmpty()  || team == QStringLiteral("t")  || team == QStringLiteral("any"));
    }

    // Difficulty (index == value)
    m_difficulty->setCurrentIndex(qBound(0, cfg.botDifficulty, 3));

    // Chatter — find by stored string data
    for (int i = 0; i < m_chatter->count(); ++i)
    {
        if (m_chatter->itemData(i).toString() == cfg.botChatter)
        {
            m_chatter->setCurrentIndex(i);
            break;
        }
    }

    // Quota mode
    for (int i = 0; i < m_quotaMode->count(); ++i)
    {
        if (m_quotaMode->itemData(i).toString() == cfg.botQuotaMode)
        {
            m_quotaMode->setCurrentIndex(i);
            break;
        }
    }

    m_prefix->setText(cfg.botPrefix);
    m_deferToHuman->setOn(cfg.botDeferToHuman     != 0, false);
    m_joinAfterPlayer->setOn(cfg.botJoinAfterPlayer != 0, false);
    m_autoVacate->setOn(cfg.botAutoVacate          != 0, false);

    m_allowPistols->setOn(cfg.botAllowPistols           != 0, false);
    m_allowShotguns->setOn(cfg.botAllowShotguns         != 0, false);
    m_allowSubMachineGuns->setOn(cfg.botAllowSubMachineGuns != 0, false);
    m_allowRifles->setOn(cfg.botAllowRifles             != 0, false);
    m_allowSnipers->setOn(cfg.botAllowSnipers           != 0, false);
    m_allowMachineGuns->setOn(cfg.botAllowMachineGuns   != 0, false);
    m_allowGrenades->setOn(cfg.botAllowGrenades         != 0, false);
    m_allowShield->setOn(cfg.botAllowShield             != 0, false);

    // Unblock signals
    block(m_botsEnabled,        false);
    block(m_botCount,           false);
    block(m_ctBots,             false);
    block(m_tBots,              false);
    block(m_quotaMode,          false);
    block(m_difficulty,         false);
    block(m_chatter,            false);
    block(m_prefix,             false);
    block(m_deferToHuman,       false);
    block(m_joinAfterPlayer,    false);
    block(m_autoVacate,         false);
    block(m_allowPistols,       false);
    block(m_allowShotguns,      false);
    block(m_allowSubMachineGuns,false);
    block(m_allowRifles,        false);
    block(m_allowSnipers,       false);
    block(m_allowMachineGuns,   false);
    block(m_allowGrenades,      false);
    block(m_allowShield,        false);
}

void BotsPage::saveBotQuota()
{
    save(QStringLiteral("bot_quota"), m_botsEnabled->isOn() ? m_botCount->value() : 0);
}

void BotsPage::saveBotTeam()
{
    const bool ct = m_ctBots->isChecked();
    const bool t  = m_tBots->isChecked();
    QString team;
    if (ct && t)   team = QStringLiteral("any");
    else if (ct)   team = QStringLiteral("CT");
    else if (t)    team = QStringLiteral("T");
    else           team = QStringLiteral("any");
    save(QStringLiteral("bot_join_team"), team);
}

void BotsPage::save(const QString& key, const QString& value)
{
    DBG_SETTINGS(QStringLiteral("BotsPage: ") + key + QStringLiteral(" = \"") + value + u'"');
    ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(), key, value);
    emit settingChanged();
}

void BotsPage::save(const QString& key, const int value)
{
    save(key, QString::number(value));
}
