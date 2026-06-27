#include "serverSettingsPage.h"

#include <QFile>
#include <QFrame>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QSvgRenderer>
#include <QVBoxLayout>

#include "appConfig.h"
#include "debug.h"
#include "serverFiles.h"
#include "widgets/numberSpinner.h"
#include "widgets/toggleSwitch.h"

namespace
{
constexpr int PAGE_MARGIN   = 20;
constexpr int GROUP_SPACING = 16;
constexpr int ROW_SPACING   = 10;
constexpr int CTRL_SPACING  = 12;
constexpr int EYE_ICON_SIZE = 14;
constexpr int HINT_SPACING  = 2;

constexpr int MAXSPEED_MIN   = 10;
constexpr int MAXSPEED_MAX   = 99999;
constexpr int MAXSPEED_WIDTH = 110;

constexpr int TIMELIMIT_MAX  = 999;
constexpr int ROUNDTIME_MIN  = 1;
constexpr int ROUNDTIME_MAX  = 60;
constexpr int FREEZETIME_MAX = 60;
constexpr int LIMITTEAMS_MAX = 30;
constexpr int HOSTAGE_MAX    = 20;

QIcon renderSvgIcon(const QString& resource, const QColor& color, int size)
{
    QFile file(resource);
    if (file.open(QIODevice::ReadOnly) == false)
        return {};
    QByteArray data = file.readAll();
    data.replace("currentColor", color.name().toLatin1());
    QSvgRenderer renderer(data);
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    renderer.render(&p);
    return QIcon(px);
}
} // namespace

ServerSettingsPage::ServerSettingsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel(tr("Server Settings"), this);
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

    // ── Helpers ───────────────────────────────────────────────────────────────

    // Row: [label + optional grey hint  |  ToggleSwitch]
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
            QLabel* hintLbl = new QLabel(hint, content);
            hintLbl->setStyleSheet(QStringLiteral("color: #888;"));
            labelCol->addWidget(hintLbl);
        }
        row->addLayout(labelCol, 1);

        ToggleSwitch* toggle = new ToggleSwitch(content);
        row->addWidget(toggle, 0, Qt::AlignVCenter);
        grp->addLayout(row);
        return toggle;
    };

    // Row: [label + optional grey hint  |  NumberSpinner]
    auto addSpinnerRow = [&](QVBoxLayout* grp, const QString& label,
                             const QString& hint = {}) -> NumberSpinner*
    {
        QHBoxLayout* row = new QHBoxLayout;
        row->setSpacing(CTRL_SPACING);

        QVBoxLayout* labelCol = new QVBoxLayout;
        labelCol->setSpacing(HINT_SPACING);
        labelCol->setContentsMargins(0, 0, 0, 0);
        labelCol->addWidget(new QLabel(label, content));
        if (hint.isEmpty() == false)
        {
            QLabel* hintLbl = new QLabel(hint, content);
            hintLbl->setStyleSheet(QStringLiteral("color: #888;"));
            labelCol->addWidget(hintLbl);
        }
        row->addLayout(labelCol, 1);

        NumberSpinner* spinner = new NumberSpinner(content);
        row->addWidget(spinner, 0, Qt::AlignVCenter);
        grp->addLayout(row);
        return spinner;
    };

    // ── Server Identity ───────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Server"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(ROW_SPACING);

        // Hostname
        grp->addWidget(new QLabel(tr("Server name:"), group));
        m_hostname = new QLineEdit(group);
        m_hostname->setPlaceholderText(tr("e.g. My CS Server"));
        grp->addWidget(m_hostname);

        // Password
        grp->addWidget(new QLabel(tr("Password:"), group));
        m_password = new QLineEdit(group);
        m_password->setPlaceholderText(tr("Leave empty for public server"));
        m_password->setEchoMode(QLineEdit::Password);
        const QColor iconColor = QGuiApplication::palette().color(QPalette::WindowText);
        m_passwordEyeAct = m_password->addAction(
            renderSvgIcon(QStringLiteral(":/assets/eye.svg"), iconColor, EYE_ICON_SIZE),
            QLineEdit::TrailingPosition);
        m_passwordEyeAct->setToolTip(tr("Show / hide password"));
        connect(m_passwordEyeAct, &QAction::triggered, this, [this]()
        {
            const bool nowHidden = (m_password->echoMode() == QLineEdit::Password);
            m_password->setEchoMode(nowHidden ? QLineEdit::Normal : QLineEdit::Password);
            const QColor col = QGuiApplication::palette().color(QPalette::WindowText);
            m_passwordEyeAct->setIcon(renderSvgIcon(
                nowHidden ? QStringLiteral(":/assets/eye-slash.svg")
                          : QStringLiteral(":/assets/eye.svg"),
                col, EYE_ICON_SIZE));
        });
        grp->addWidget(m_password);

        // LAN only
        m_svLan = addToggleRow(grp, tr("LAN only"),
                               tr("Restrict the server to the local network (sv_lan)"));

        // Region
        grp->addWidget(new QLabel(tr("Region:"), group));
        m_svRegion = new QComboBox(group);
        m_svRegion->addItem(tr("World"),          -1);
        m_svRegion->addItem(tr("US East coast"),   0);
        m_svRegion->addItem(tr("US West coast"),   1);
        m_svRegion->addItem(tr("South America"),   2);
        m_svRegion->addItem(tr("Europe"),          3);
        m_svRegion->addItem(tr("Asia"),            4);
        m_svRegion->addItem(tr("Australia"),       5);
        m_svRegion->addItem(tr("Middle East"),     6);
        m_svRegion->addItem(tr("Africa"),          7);
        grp->addWidget(m_svRegion);

        contentLayout->addWidget(group);

        // Connections
        connect(m_hostname, &QLineEdit::editingFinished, this, [this]()
        {
            save(QStringLiteral("hostname"), m_hostname->text().trimmed());
        });
        connect(m_password, &QLineEdit::editingFinished, this, [this]()
        {
            save(QStringLiteral("sv_password"), m_password->text());
        });
        connect(m_svLan, &ToggleSwitch::toggled, this, [this](bool on)
        {
            save(QStringLiteral("sv_lan"), on ? 1 : 0);
        });
        connect(m_svRegion, &QComboBox::currentIndexChanged, this, [this]()
        {
            save(QStringLiteral("sv_region"), m_svRegion->currentData().toInt());
        });
    }

    // ── Gameplay ──────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Gameplay"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(ROW_SPACING);

        m_mpTimelimit = addSpinnerRow(grp, tr("Time limit"),
                                     tr("Minutes per map (0 = unlimited)"));
        m_mpTimelimit->setRange(0, TIMELIMIT_MAX);

        m_mpRoundtime = addSpinnerRow(grp, tr("Round time"),
                                     tr("Minutes per round"));
        m_mpRoundtime->setRange(ROUNDTIME_MIN, ROUNDTIME_MAX);

        m_mpFreezetime = addSpinnerRow(grp, tr("Freeze time"),
                                      tr("Seconds at round start before movement"));
        m_mpFreezetime->setRange(0, FREEZETIME_MAX);

        m_mpFriendlyfire = addToggleRow(grp, tr("Friendly fire"),
                                        tr("Allow players to damage teammates"));

        m_mpAutobalance = addToggleRow(grp, tr("Auto team balance"),
                                       tr("Force players to even up teams"));

        m_mpLimitteams = addSpinnerRow(grp, tr("Team size limit"),
                                       tr("Max difference between team sizes (0 = no limit)"));
        m_mpLimitteams->setRange(0, LIMITTEAMS_MAX);

        m_mpTkpunish = addToggleRow(grp, tr("Punish team kills"),
                                    tr("Punish players for killing teammates"));

        m_mpHostagepenalty = addSpinnerRow(grp, tr("Hostage kill limit"),
                                           tr("Kick a terrorist after this many hostage kills (0 = disabled)"));
        m_mpHostagepenalty->setRange(0, HOSTAGE_MAX);

        m_mpFlashlight = addToggleRow(grp, tr("Flashlight"),
                                      tr("Allow players to use the flashlight"));

        m_mpFootsteps = addToggleRow(grp, tr("Footsteps"),
                                     tr("Players can hear each other's footsteps"));

        contentLayout->addWidget(group);

        // Connections
        connect(m_mpTimelimit,  &NumberSpinner::valueChanged, this,
                [this](int v) { save(QStringLiteral("mp_timelimit"),  v); });
        connect(m_mpRoundtime,  &NumberSpinner::valueChanged, this,
                [this](int v) { save(QStringLiteral("mp_roundtime"),  v); });
        connect(m_mpFreezetime, &NumberSpinner::valueChanged, this,
                [this](int v) { save(QStringLiteral("mp_freezetime"), v); });
        connect(m_mpFriendlyfire, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("mp_friendlyfire"), on ? 1 : 0); });
        connect(m_mpAutobalance, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("mp_autoteambalance"), on ? 1 : 0); });
        connect(m_mpLimitteams, &NumberSpinner::valueChanged, this,
                [this](int v) { save(QStringLiteral("mp_limitteams"), v); });
        connect(m_mpTkpunish, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("mp_tkpunish"), on ? 1 : 0); });
        connect(m_mpHostagepenalty, &NumberSpinner::valueChanged, this,
                [this](int v) { save(QStringLiteral("mp_hostagepenalty"), v); });
        connect(m_mpFlashlight, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("mp_flashlight"), on ? 1 : 0); });
        connect(m_mpFootsteps, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("mp_footsteps"), on ? 1 : 0); });
    }

    // ── Advanced ──────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Advanced"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(ROW_SPACING);

        m_svMaxspeed = addSpinnerRow(grp, tr("Max player speed"),
                                     tr("Maximum movement speed (default 320)"));
        m_svMaxspeed->setRange(MAXSPEED_MIN, MAXSPEED_MAX);
        m_svMaxspeed->setFixedWidth(MAXSPEED_WIDTH);

        m_svCheats = addToggleRow(grp, tr("Allow cheats"),
                                  tr("Enable cheat commands (sv_cheats)"));

        m_svAim = addToggleRow(grp, tr("Allow auto-aim"),
                               tr("Enable built-in auto-aim assist (sv_aim)"));

        m_svPausable = addToggleRow(grp, tr("Allow pausing"),
                                    tr("Let clients pause the server (sv_pausable)"));

        contentLayout->addWidget(group);

        connect(m_svMaxspeed, &NumberSpinner::valueChanged, this,
                [this](int v) { save(QStringLiteral("sv_maxspeed"), v); });
        connect(m_svCheats, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("sv_cheats"), on ? 1 : 0); });
        connect(m_svAim, &ToggleSwitch::toggled, this,
                [this](bool on) { save(QStringLiteral("sv_aim"), on ? 1 : 0); });
        connect(m_svPausable, &ToggleSwitch::toggled, this, [this](bool on)
        {
            const int v = on ? 1 : 0;
            save(QStringLiteral("sv_pausable"), v);
            save(QStringLiteral("pausable"), v);
        });
    }

    contentLayout->addStretch();

    loadForGame(AppConfig::instance().selectedGame());
}

void ServerSettingsPage::loadForGame(const AppConfig::Game game)
{
    const ServerFiles::ServerConfig cfg = ServerFiles::readServerConfig(game);

    // Block signals so none of the control updates trigger a save.
    const auto block = [](QObject* obj, bool b) { obj->blockSignals(b); };
    const bool on = true;

    block(m_hostname,         on);
    block(m_password,         on);
    block(m_svLan,            on);
    block(m_svRegion,         on);
    block(m_mpTimelimit,      on);
    block(m_mpRoundtime,      on);
    block(m_mpFreezetime,     on);
    block(m_mpFlashlight,     on);
    block(m_mpFootsteps,      on);
    block(m_mpFriendlyfire,   on);
    block(m_mpAutobalance,    on);
    block(m_mpLimitteams,     on);
    block(m_mpTkpunish,       on);
    block(m_mpHostagepenalty, on);
    block(m_svMaxspeed,       on);
    block(m_svCheats,         on);
    block(m_svAim,            on);
    block(m_svPausable,       on);

    m_hostname->setText(cfg.hostname);
    m_password->setText(cfg.password);

    m_svLan->setOn(cfg.svLan != 0, false);

    // Select the combo entry whose stored data matches cfg.svRegion.
    for (int i = 0; i < m_svRegion->count(); ++i)
    {
        if (m_svRegion->itemData(i).toInt() == cfg.svRegion)
        {
            m_svRegion->setCurrentIndex(i);
            break;
        }
    }

    m_mpTimelimit->setValue(cfg.mpTimelimit);
    m_mpRoundtime->setValue(cfg.mpRoundtime);
    m_mpFreezetime->setValue(cfg.mpFreezetime);

    m_mpFlashlight->setOn(cfg.mpFlashlight   != 0, false);
    m_mpFootsteps->setOn(cfg.mpFootsteps     != 0, false);
    m_mpFriendlyfire->setOn(cfg.mpFriendlyfire != 0, false);
    m_mpAutobalance->setOn(cfg.mpAutoteambalance != 0, false);
    m_mpTkpunish->setOn(cfg.mpTkpunish       != 0, false);

    m_mpLimitteams->setValue(cfg.mpLimitteams);
    m_mpHostagepenalty->setValue(cfg.mpHostagepenalty);

    m_svMaxspeed->setValue(cfg.svMaxspeed);
    m_svCheats->setOn(cfg.svCheats != 0, false);
    m_svAim->setOn(cfg.svAim       != 0, false);
    m_svPausable->setOn(cfg.svPausable != 0, false);

    // Unblock signals.
    block(m_hostname,         !on);
    block(m_password,         !on);
    block(m_svLan,            !on);
    block(m_svRegion,         !on);
    block(m_mpTimelimit,      !on);
    block(m_mpRoundtime,      !on);
    block(m_mpFreezetime,     !on);
    block(m_mpFlashlight,     !on);
    block(m_mpFootsteps,      !on);
    block(m_mpFriendlyfire,   !on);
    block(m_mpAutobalance,    !on);
    block(m_mpLimitteams,     !on);
    block(m_mpTkpunish,       !on);
    block(m_mpHostagepenalty, !on);
    block(m_svMaxspeed,       !on);
    block(m_svCheats,         !on);
    block(m_svAim,            !on);
    block(m_svPausable,       !on);
}

void ServerSettingsPage::save(const QString& key, const QString& value)
{
    DBG_SETTINGS(QStringLiteral("ServerSettings: ") + key + QStringLiteral(" = \"") + value + u'"');
    ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(), key, value);
    emit settingChanged();
}

void ServerSettingsPage::save(const QString& key, const int value)
{
    save(key, QString::number(value));
}
