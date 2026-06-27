#include "serverPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

#include <QAction>
#include <QApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QEvent>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

#include "cli/flatpakUtils.h"
#include "firewallHelpDialog.h"
#include "serverFiles.h"
#include "widgets/htmlHighlighter.h"
#include "serverUtils.h"
#include "widgets/numberSpinner.h"
#include "widgets/toggleWithStatus.h"

namespace
{
constexpr int EYE_ICON_SIZE        = 14;
constexpr int CONTENT_MARGIN       = 20;
constexpr int GROUP_SPACING        = 16;
constexpr int FORM_SPACING         = 8;
constexpr int PORT_ROW_SPACING     = 6;
constexpr int MOTD_TAB_MIN_HEIGHT  = 130; // editor content height + tab bar
constexpr int PORT_SPINNER_WIDTH   = 110;

constexpr int PORT_MIN     = 1;
constexpr int PORT_MAX     = 65535;

constexpr int MAX_PLAYERS_MIN     = 1;
constexpr int MAX_PLAYERS_MAX     = 64;
constexpr int MAX_PLAYERS_DEFAULT = 20;

constexpr int TIME_LIMIT_MIN     = 0;
constexpr int TIME_LIMIT_MAX     = 999;
constexpr int TIME_LIMIT_DEFAULT = 30;

constexpr int BOT_COUNT_MIN     = 0;
constexpr int BOT_COUNT_MAX     = 32;
constexpr int BOT_COUNT_DEFAULT = 5;

constexpr int BOT_TEAM_SPACING = 20;

QIcon renderSvgIcon(const QString& resource, const QColor& color, const int size)
{
    QFile file(resource);
    if (file.open(QIODevice::ReadOnly) == false)
        return {};
    QByteArray data = file.readAll();
    data.replace("currentColor", color.name().toLatin1());
    QSvgRenderer renderer(data);
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter painter(&px);
    renderer.render(&painter);
    return QIcon(px);
}

constexpr int TOOLTIP_ICON_SIZE = 14;
constexpr int TOOLTIP_SPACING   = 4;

// Creates a small question-circle icon label with the given tooltip text.
QLabel* makeTooltipIcon(const QString& tip, QWidget* parent)
{
    const QColor color = QApplication::palette().color(QPalette::PlaceholderText);
    const QIcon icon = renderSvgIcon(
        QStringLiteral(":/assets/question-circle.svg"), color, TOOLTIP_ICON_SIZE);
    QLabel* lbl = new QLabel(parent);
    lbl->setPixmap(icon.pixmap(TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE));
    lbl->setToolTip(tip);
    lbl->setCursor(Qt::WhatsThisCursor);
    lbl->setFixedSize(TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE);
    return lbl;
}

// Creates a form-row label widget: [label text] [? icon].
// Use as the first argument to QFormLayout::addRow() so the icon sits
// right next to the label text rather than far away by the input field.
QWidget* makeLabelWithTooltip(const QString& text, const QString& tip, QWidget* parent)
{
    QWidget* w = new QWidget(parent);
    QHBoxLayout* hl = new QHBoxLayout(w);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(TOOLTIP_SPACING);
    hl->addWidget(new QLabel(text, w));
    hl->addWidget(makeTooltipIcon(tip, w));
    return w;
}
} // namespace

ServerPage::ServerPage(QWidget* parent) : QWidget(parent)
{
    m_firewallChecker = new FirewallChecker(this);
    connect(m_firewallChecker, &FirewallChecker::resultReady,
            this, &ServerPage::onFirewallResult);


    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel(tr("Server Overview"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    outerLayout->addWidget(titleLabel);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scroll, 1);

    QWidget* content = new QWidget(scroll);
    scroll->setWidget(content);

    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);
    layout->setSpacing(GROUP_SPACING);

    // ── Server Identity ───────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Server Identity"), content);
        QFormLayout* form = new QFormLayout(group);
        form->setSpacing(FORM_SPACING);

        m_hostnameEdit = new QLineEdit(group);
        m_hostnameEdit->setPlaceholderText(tr("My CS Server"));
        form->addRow(
            makeLabelWithTooltip(tr("Hostname:"),
                tr("The server name shown in the in-game browser.\n"
                   "Example: My CS Server"), group),
            m_hostnameEdit);

        m_passwordEdit = new QLineEdit(group);
        m_passwordEdit->setPlaceholderText(tr("Leave empty for public server"));
        m_passwordEdit->setEchoMode(QLineEdit::Password);

        m_passwordToggleAction = m_passwordEdit->addAction(
            renderSvgIcon(QStringLiteral(":/assets/eye.svg"),
                          palette().color(QPalette::PlaceholderText), EYE_ICON_SIZE),
            QLineEdit::TrailingPosition);
        m_passwordToggleAction->setToolTip(tr("Show password"));

        // QLineEdit::addAction() creates an internal QToolButton — set the
        // cursor on it so hovering shows a pointer rather than the text cursor.
        for (QToolButton* btn : m_passwordEdit->findChildren<QToolButton*>())
        {
            btn->setCursor(Qt::PointingHandCursor);
        }

        connect(m_passwordToggleAction, &QAction::triggered, this, [this]()
        {
            const bool nowVisible = (m_passwordEdit->echoMode() == QLineEdit::Password);
            m_passwordEdit->setEchoMode(nowVisible ? QLineEdit::Normal : QLineEdit::Password);
            refreshPasswordToggleIcon();
        });

        form->addRow(
            makeLabelWithTooltip(tr("Password:"),
                tr("Password required to join. Leave empty for a public server."), group),
            m_passwordEdit);

        // MOTD — tabbed: Source editor with syntax highlighting + HTML preview
        m_motdTabs = new QTabWidget(group);
        m_motdTabs->setMinimumHeight(MOTD_TAB_MIN_HEIGHT);

        m_motdEdit = new QPlainTextEdit(m_motdTabs);
        m_motdEdit->setPlaceholderText(tr("Welcome! Follow the rules and have fun."));
        new HtmlHighlighter(m_motdEdit->document()); // owned by the document

        QTextBrowser* motdPreview = new QTextBrowser(m_motdTabs);
        motdPreview->setOpenLinks(false);

        m_motdTabs->addTab(m_motdEdit, tr("Source"));
        m_motdTabs->addTab(motdPreview, tr("Preview"));

        // Refresh preview when the user switches to it
        connect(m_motdTabs, &QTabWidget::currentChanged, this,
            [this, motdPreview](const int index)
        {
            if (index == 1)
            {
                motdPreview->setHtml(m_motdEdit->toPlainText());
            }
        });

        form->addRow(
            makeLabelWithTooltip(tr("MOTD:"),
                tr("Message of the Day — HTML shown to players when they connect.\n"
                   "File: czero/motd.txt  |  Basic HTML and inline CSS supported."), group),
            m_motdTabs);

        layout->addWidget(group);
    }

    // ── Connection ────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Connection"), content);
        QFormLayout* form = new QFormLayout(group);
        form->setSpacing(FORM_SPACING);

        // IP row: [text field] [Detect button]
        QWidget* ipRow = new QWidget(group);
        QHBoxLayout* ipLayout = new QHBoxLayout(ipRow);
        ipLayout->setContentsMargins(0, 0, 0, 0);
        ipLayout->setSpacing(PORT_ROW_SPACING);

        m_ipEdit = new QLineEdit(ipRow);
        m_ipEdit->setPlaceholderText(QStringLiteral("0.0.0.0"));
        ipLayout->addWidget(m_ipEdit, 1);

        m_detectIpBtn = new QPushButton(tr("Detect"), ipRow);
        m_detectIpBtn->setFocusPolicy(Qt::NoFocus);
        m_detectIpBtn->setToolTip(
            tr("Run ifconfig and fill in your local IP address.\n"
               "The detected address is saved for next launch."));
        ipLayout->addWidget(m_detectIpBtn);

        form->addRow(
            makeLabelWithTooltip(tr("IP Address:"),
                tr("IP address the server binds to.\n"
                   "0.0.0.0 — listen on all network interfaces (recommended)\n"
                   "Leave empty to use the system default."), group),
            ipRow);

        // Port row: spinner | Check Firewall button | status label | fix link
        QWidget* portRow = new QWidget(group);
        QHBoxLayout* portLayout = new QHBoxLayout(portRow);
        portLayout->setContentsMargins(0, 0, 0, 0);
        portLayout->setSpacing(PORT_ROW_SPACING);

        m_portSpinner = new NumberSpinner(portRow);
        m_portSpinner->setRange(PORT_MIN, PORT_MAX);
        m_portSpinner->setFixedWidth(PORT_SPINNER_WIDTH);
        portLayout->addWidget(m_portSpinner);

        m_firewallCheckBtn = new QPushButton(tr("Check Firewall"), portRow);
        m_firewallCheckBtn->setObjectName(QStringLiteral("firewallCheckBtn"));
        m_firewallCheckBtn->setToolTip(
            tr("Checks whether the server port is open in firewalld or ufw.\n"
               "May require your sudo password."));
        portLayout->addWidget(m_firewallCheckBtn);

        m_firewallLabel = new QLabel(portRow);
        m_firewallLabel->setObjectName(QStringLiteral("firewallStatusLabel"));
        m_firewallLabel->setVisible(false);
        portLayout->addWidget(m_firewallLabel);

        m_firewallHelpLink = new QLabel(portRow);
        m_firewallHelpLink->setTextFormat(Qt::RichText);
        m_firewallHelpLink->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        m_firewallHelpLink->setVisible(false);
        portLayout->addWidget(m_firewallHelpLink);

        portLayout->addStretch();
        form->addRow(
            makeLabelWithTooltip(tr("Port:"),
                tr("The network port clients use to connect.\n"
                   "Default: 27015\n"
                   "Ports below 1024 require root privileges."), group),
            portRow);

        layout->addWidget(group);

        connect(m_detectIpBtn, &QPushButton::clicked, this, &ServerPage::detectLocalIp);

        // Persist IP on focus-out / Enter
        connect(m_ipEdit, &QLineEdit::editingFinished, this, [this]()
        {
            const AppConfig::Game game = AppConfig::instance().selectedGame();
            const QString ip = m_ipEdit->text().trimmed();
            if (game == AppConfig::Game::CZ)
            {
                AppConfig::instance().setCzIp(ip);
            }
            else
            {
                AppConfig::instance().setCs16Ip(ip);
            }
        });

        // Persist port on change; clear any stale firewall result
        connect(m_portSpinner, &NumberSpinner::valueChanged, this, [this](const int value)
        {
            const AppConfig::Game game = AppConfig::instance().selectedGame();
            if (game == AppConfig::Game::CZ)
            {
                AppConfig::instance().setCzPort(value);
            }
            else
            {
                AppConfig::instance().setCs16Port(value);
            }
            m_firewallLabel->setVisible(false);
            m_firewallHelpLink->setVisible(false);
        });

        // Manual check button — use checkNow() to skip the debounce delay
        connect(m_firewallCheckBtn, &QPushButton::clicked, this, [this]()
        {
            m_firewallLabel->setVisible(false);
            m_firewallHelpLink->setVisible(false);
            m_firewallChecker->checkNow(m_portSpinner->value());
        });

        // Fix-link opens the help dialog
        connect(m_firewallHelpLink, &QLabel::linkActivated, this, [this](const QString&)
        {
            FirewallHelpDialog dlg(m_portSpinner->value(), m_lastFirewallType, this);
            dlg.exec();
        });
    }

    // ── Gameplay ──────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Gameplay"), content);
        QFormLayout* form = new QFormLayout(group);
        form->setSpacing(FORM_SPACING);

        m_mapCombo = new QComboBox(group);
        form->addRow(tr("Map:"), m_mapCombo);

        m_maxPlayersSpinner = new NumberSpinner(group);
        m_maxPlayersSpinner->setRange(MAX_PLAYERS_MIN, MAX_PLAYERS_MAX);
        m_maxPlayersSpinner->setValue(MAX_PLAYERS_DEFAULT);
        form->addRow(
            makeLabelWithTooltip(tr("Max Players:"),
                tr("Maximum number of simultaneous players.\nRange: 1–64"), group),
            m_maxPlayersSpinner);

        m_timeLimitSpinner = new NumberSpinner(group);
        m_timeLimitSpinner->setRange(TIME_LIMIT_MIN, TIME_LIMIT_MAX);
        m_timeLimitSpinner->setValue(TIME_LIMIT_DEFAULT);
        form->addRow(
            makeLabelWithTooltip(tr("Time Limit (min):"),
                tr("Minutes each map plays before rotating.\n"
                   "mp_timelimit — 0 = no time limit"), group),
            m_timeLimitSpinner);

        layout->addWidget(group);
    }

    // ── Bots ──────────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Bots"), content);
        m_botsGroup = group;
        QVBoxLayout* groupLayout = new QVBoxLayout(group);
        groupLayout->setSpacing(FORM_SPACING);

        QHBoxLayout* toggleRow = new QHBoxLayout();
        toggleRow->setContentsMargins(0, 0, 0, 0);
        m_botsToggle = new ToggleWithStatus(group);
        m_botsToggle->setOn(false, false);
        toggleRow->addWidget(makeLabelWithTooltip(
            tr("Enable Bots:"),
            tr("Add AI bot players to fill empty player slots."), group));
        toggleRow->addWidget(m_botsToggle);
        toggleRow->addStretch();
        groupLayout->addLayout(toggleRow);

        m_botOptions = new QWidget(group);
        m_botOptions->setVisible(false);
        QFormLayout* botForm = new QFormLayout(m_botOptions);
        botForm->setContentsMargins(0, 0, 0, 0);
        botForm->setSpacing(FORM_SPACING);

        m_botCountSpinner = new NumberSpinner(m_botOptions);
        m_botCountSpinner->setRange(BOT_COUNT_MIN, BOT_COUNT_MAX);
        m_botCountSpinner->setValue(BOT_COUNT_DEFAULT);
        botForm->addRow(
            makeLabelWithTooltip(tr("Bot Count:"),
                tr("Target number of bot players on the server.\n"
                   "bot_quota — 0 = no bots even when enabled."), m_botOptions),
            m_botCountSpinner);

        QWidget* teamWidget = new QWidget(m_botOptions);
        QHBoxLayout* teamLayout = new QHBoxLayout(teamWidget);
        teamLayout->setContentsMargins(0, 0, 0, 0);
        teamLayout->setSpacing(BOT_TEAM_SPACING);
        m_ctBotsCheck = new QCheckBox(tr("Counter-Terrorist"), teamWidget);
        m_ctBotsCheck->setChecked(true);
        m_ctBotsCheck->setToolTip(tr("Allow bots to join the Counter-Terrorist team."));
        m_tBotsCheck = new QCheckBox(tr("Terrorist"), teamWidget);
        m_tBotsCheck->setChecked(true);
        m_tBotsCheck->setToolTip(tr("Allow bots to join the Terrorist team."));
        teamLayout->addWidget(m_ctBotsCheck);
        teamLayout->addWidget(m_tBotsCheck);
        teamLayout->addStretch();
        botForm->addRow(tr("Bot Teams:"), teamWidget);

        groupLayout->addWidget(m_botOptions);
        layout->addWidget(group);

        connect(m_botsToggle, &ToggleWithStatus::toggled, m_botOptions, &QWidget::setVisible);
    }

    layout->addStretch();

    // ── Auto-save server files on edit ────────────────────────────────────────
    // Set up once; always use AppConfig::selectedGame() at save time so changes
    // are written to whichever game is currently active.

    // ── Gameplay settings auto-save ───────────────────────────────────────────
    connect(m_maxPlayersSpinner, &NumberSpinner::valueChanged, this, [this](const int value)
    {
        const AppConfig::Game game = AppConfig::instance().selectedGame();
        if (game == AppConfig::Game::CZ)
            AppConfig::instance().setCzMaxPlayers(value);
        else
            AppConfig::instance().setCs16MaxPlayers(value);
        emit settingChanged();
    });

    // Time limit and bots write directly to server.cfg (not AppConfig).
    connect(m_timeLimitSpinner, &NumberSpinner::valueChanged, this, [this](const int value)
    {
        ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(),
                                            QStringLiteral("mp_timelimit"),
                                            QString::number(value));
        emit settingChanged();
    });

    connect(m_botsToggle, &ToggleWithStatus::toggled, this, [this](const bool on)
    {
        // Toggle ON  → write the current bot count; OFF → write 0.
        const int quota = on ? m_botCountSpinner->value() : 0;
        ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(),
                                            QStringLiteral("bot_quota"),
                                            QString::number(quota));
        emit settingChanged();
    });

    connect(m_botCountSpinner, &NumberSpinner::valueChanged, this, [this](const int value)
    {
        // Only update bot_quota while bots are enabled.
        if (m_botsToggle->isOn() == false) return;
        ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(),
                                            QStringLiteral("bot_quota"),
                                            QString::number(value));
        emit settingChanged();
    });

    connect(m_ctBotsCheck, &QCheckBox::toggled, this, [this](bool)
    {
        writeBotsTeamToConfig();
    });

    connect(m_tBotsCheck, &QCheckBox::toggled, this, [this](bool)
    {
        writeBotsTeamToConfig();
    });

    connect(m_mapCombo, &QComboBox::currentTextChanged, this, [this](const QString& text)
    {
        if (text.isEmpty()) return; // fired by clear() — not a real selection
        const AppConfig::Game game = AppConfig::instance().selectedGame();
        if (game == AppConfig::Game::CZ)
            AppConfig::instance().setCzStartMap(text);
        else
            AppConfig::instance().setCs16StartMap(text);
        emit settingChanged();
    });

    connect(m_hostnameEdit, &QLineEdit::editingFinished, this, [this]()
    {
        const QString current = m_hostnameEdit->text();
        if (current == m_loadedHostname) return;
        if (ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(),
                                                QStringLiteral("hostname"), current))
        {
            m_loadedHostname = current;
            emit settingChanged();
        }
    });

    connect(m_passwordEdit, &QLineEdit::editingFinished, this, [this]()
    {
        const QString current = m_passwordEdit->text();
        if (current == m_loadedPassword) return;
        if (ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(),
                                                QStringLiteral("sv_password"), current))
        {
            m_loadedPassword = current;
            emit settingChanged();
        }
    });

    // QPlainTextEdit has no editingFinished, so detect focus-out via
    // QApplication::focusChanged. Walk the parent chain so we catch the
    // internal viewport losing focus too.
    connect(qApp, &QApplication::focusChanged, this,
        [this](QWidget* old, QWidget* now)
    {
        const bool wasInMotd = (old != nullptr)
            && (old == m_motdEdit || m_motdEdit->isAncestorOf(old));
        const bool stillInMotd = (now != nullptr)
            && (now == m_motdEdit || m_motdEdit->isAncestorOf(now));

        if (wasInMotd == false || stillInMotd) return;

        const QString current = m_motdEdit->toPlainText();
        if (current == m_loadedMotd) return;
        if (ServerFiles::writeMotd(AppConfig::instance().selectedGame(), current))
        {
            m_loadedMotd = current;
            emit settingChanged();
        }
    });

    // Load values for the initially selected game
    loadForGame(AppConfig::instance().selectedGame());
}

void ServerPage::loadForGame(const AppConfig::Game game)
{
    // CS 1.6 has no native bot support — hide the whole bots section.
    m_botsGroup->setVisible(game == AppConfig::Game::CZ);
    const QString ip   = (game == AppConfig::Game::CZ)
        ? AppConfig::instance().czIp()
        : AppConfig::instance().cs16Ip();
    const int     port = (game == AppConfig::Game::CZ)
        ? AppConfig::instance().czPort()
        : AppConfig::instance().cs16Port();

    m_ipEdit->setText(ip);
    m_portSpinner->setValue(port);

    // ── Gameplay settings (always loaded from AppConfig, no file I/O needed) ─
    m_maxPlayersSpinner->setValue((game == AppConfig::Game::CZ)
        ? AppConfig::instance().czMaxPlayers()
        : AppConfig::instance().cs16MaxPlayers());

    // Time limit and bots are read from server.cfg below (inside the valid-path block).

    // ── Server files (MOTD + server.cfg) ─────────────────────────────────────
    const QString serverPath = (game == AppConfig::Game::CZ)
        ? AppConfig::instance().czServerPath()
        : AppConfig::instance().cs16ServerPath();

    m_motdTabs->setCurrentIndex(0); // always show Source after a load

    if (isValidServerPath(serverPath))
    {
        // Read the saved map BEFORE repopulating the combo. addItems() fires
        // currentTextChanged synchronously, which would otherwise overwrite the
        // saved value in AppConfig before we get a chance to read it.
        const QString savedMap = (game == AppConfig::Game::CZ)
            ? AppConfig::instance().czStartMap()
            : AppConfig::instance().cs16StartMap();

        const QStringList maps = ServerFiles::scanMaps(game);
        m_mapCombo->blockSignals(true);
        m_mapCombo->clear();
        m_mapCombo->addItems(maps);
        const int savedIdx = m_mapCombo->findText(savedMap, Qt::MatchFixedString);
        // Fall back to the first map if the saved one is no longer present
        m_mapCombo->setCurrentIndex(savedIdx >= 0 ? savedIdx : 0);
        m_mapCombo->blockSignals(false);

        m_loadedMotd = ServerFiles::readMotd(game);
        m_motdEdit->setPlainText(m_loadedMotd);
        // Deferred repaint — needed when the page is inside a hidden QStackedWidget.
        QTimer::singleShot(0, m_motdEdit->viewport(), [this]() { m_motdEdit->viewport()->update(); });

        const ServerFiles::ServerConfig cfg = ServerFiles::readServerConfig(game);
        m_loadedHostname = cfg.hostname;
        m_loadedPassword = cfg.password;
        if (m_loadedHostname.isEmpty() == false)
        {
            m_hostnameEdit->setText(m_loadedHostname);
        }
        if (m_loadedPassword.isEmpty() == false)
        {
            m_passwordEdit->setText(m_loadedPassword);
        }

        // Time limit
        if (cfg.mpTimelimit >= 0)
        {
            m_timeLimitSpinner->setValue(cfg.mpTimelimit);
        }

        // Bots: bot_quota 0 → disabled, > 0 → enabled with that count
        if (cfg.botQuota >= 0)
        {
            const bool botsOn = (cfg.botQuota > 0);
            m_botsToggle->setOn(botsOn, false);
            m_botOptions->setVisible(botsOn);
            if (botsOn)
            {
                m_botCountSpinner->setValue(cfg.botQuota);
            }
        }

        // bot_join_team: "T" → T only, "CT" → CT only, "any"/missing → both
        if (cfg.botJoinTeam.isEmpty() == false)
        {
            const QString team = cfg.botJoinTeam.toLower();
            m_ctBotsCheck->setChecked(team == QStringLiteral("ct") || team == QStringLiteral("any"));
            m_tBotsCheck->setChecked(team == QStringLiteral("t")  || team == QStringLiteral("any"));
        }
    }
    else
    {
        m_mapCombo->clear();
        m_loadedHostname.clear();
        m_loadedPassword.clear();
        m_loadedMotd.clear();
        m_motdEdit->clear();
        m_hostnameEdit->clear();
        m_passwordEdit->clear();
    }

    // Clear any stale firewall result — user can re-check with the button
    m_firewallLabel->setVisible(false);
    m_firewallHelpLink->setVisible(false);
}

void ServerPage::onFirewallResult(const int port,
                                   const FirewallChecker::Status status,
                                   const FirewallChecker::FirewallType type)
{
    // Ignore results for a port that's no longer current
    if (port != m_portSpinner->value()) return;

    m_lastFirewallType = type;

    switch (status)
    {
        case FirewallChecker::Status::Allowed:
            m_firewallLabel->setText(tr("✓  Port %1 is open (TCP & UDP)").arg(port));
            m_firewallLabel->setProperty("firewallStatus", QStringLiteral("ok"));
            m_firewallLabel->setVisible(true);
            m_firewallHelpLink->setVisible(false);
            break;

        case FirewallChecker::Status::PartiallyAllowed:
            m_firewallLabel->setText(tr("⚠  Port %1 is only partially open").arg(port));
            m_firewallLabel->setProperty("firewallStatus", QStringLiteral("warn"));
            m_firewallLabel->setVisible(true);
            m_firewallHelpLink->setText(tr("<a href=\"fix\">How to fully open port %1</a>").arg(port));
            m_firewallHelpLink->setVisible(true);
            break;

        case FirewallChecker::Status::Blocked:
            m_firewallLabel->setText(tr("✗  Port %1 is not open").arg(port));
            m_firewallLabel->setProperty("firewallStatus", QStringLiteral("blocked"));
            m_firewallLabel->setVisible(true);
            m_firewallHelpLink->setText(tr("<a href=\"fix\">How to open port %1</a>").arg(port));
            m_firewallHelpLink->setVisible(true);
            break;

        case FirewallChecker::Status::Unknown:
            m_firewallLabel->setText(tr("Could not detect ufw or firewalld on this system"));
            m_firewallLabel->setProperty("firewallStatus", QStringLiteral("unknown"));
            m_firewallLabel->setVisible(true);
            m_firewallHelpLink->setVisible(false);
            break;

        default:
            m_firewallLabel->setVisible(false);
            m_firewallHelpLink->setVisible(false);
            break;
    }

    m_firewallLabel->style()->unpolish(m_firewallLabel);
    m_firewallLabel->style()->polish(m_firewallLabel);
}

QString ServerPage::currentIp()   const { return m_ipEdit->text(); }
int     ServerPage::currentPort() const { return m_portSpinner->value(); }
QString ServerPage::currentMap()  const { return m_mapCombo->currentText(); }
int     ServerPage::maxPlayers()  const { return m_maxPlayersSpinner->value(); }

void ServerPage::setStartMap(const QString& map)
{
    const int idx = m_mapCombo->findText(map, Qt::MatchFixedString);
    if (idx < 0) return;
    m_mapCombo->blockSignals(true);
    m_mapCombo->setCurrentIndex(idx);
    m_mapCombo->blockSignals(false);
}

void ServerPage::refreshPasswordToggleIcon()
{
    if (m_passwordToggleAction == nullptr) return;

    const bool hidden = (m_passwordEdit->echoMode() == QLineEdit::Password);
    const QString resource = hidden
        ? QStringLiteral(":/assets/eye.svg")
        : QStringLiteral(":/assets/eye-slash.svg");
    const QColor color = palette().color(QPalette::PlaceholderText);

    m_passwordToggleAction->setIcon(renderSvgIcon(resource, color, EYE_ICON_SIZE));
    m_passwordToggleAction->setToolTip(hidden ? tr("Show password") : tr("Hide password"));
}

void ServerPage::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
    {
        refreshPasswordToggleIcon();
    }
}

void ServerPage::detectLocalIp()
{
    m_detectIpBtn->setEnabled(false);
    m_detectIpBtn->setText(tr("Detecting…"));

    auto [prog, args] = buildHostCommand(QStringLiteral("ifconfig"), {});
    QProcess* proc = new QProcess(this);

    // Restore the button regardless of success or failure.
    auto restoreBtn = [this]()
    {
        m_detectIpBtn->setText(tr("Detect"));
        m_detectIpBtn->setEnabled(true);
    };

    connect(proc, &QProcess::finished, this, [this, proc, restoreBtn](int, QProcess::ExitStatus)
    {
        const QString output = QString::fromLocal8Bit(proc->readAllStandardOutput());
        proc->deleteLater();

        // Match "inet <IPv4>" lines; \b prevents matching "inet6".
        const QRegularExpression re(QStringLiteral("\\binet\\s+([\\d.]+)"));
        QRegularExpressionMatchIterator it = re.globalMatch(output);

        QString foundIp;
        while (it.hasNext())
        {
            const QRegularExpressionMatch match = it.next();
            const QString ip = match.captured(1);
            if (ip.startsWith(QStringLiteral("127.")) == false)
            {
                foundIp = ip;
                break;
            }
        }

        if (foundIp.isEmpty() == false)
        {
            m_ipEdit->setText(foundIp);

            // Save immediately — same path as manual editingFinished.
            const AppConfig::Game game = AppConfig::instance().selectedGame();
            if (game == AppConfig::Game::CZ)
            {
                AppConfig::instance().setCzIp(foundIp);
            }
            else
            {
                AppConfig::instance().setCs16Ip(foundIp);
            }
        }

        restoreBtn();
    });

    connect(proc, &QProcess::errorOccurred, this, [proc, restoreBtn](QProcess::ProcessError)
    {
        proc->deleteLater();
        restoreBtn();
    });

    proc->start(prog, args);
}

void ServerPage::writeBotsTeamToConfig()
{
    const bool ct = m_ctBotsCheck->isChecked();
    const bool t  = m_tBotsCheck->isChecked();

    QString teamValue;
    if (ct && t)
    {
        teamValue = QStringLiteral("any");
    }
    else if (ct)
    {
        teamValue = QStringLiteral("CT");
    }
    else if (t)
    {
        teamValue = QStringLiteral("T");
    }
    else
    {
        teamValue = QStringLiteral("any");
    }

    ServerFiles::writeServerConfigValue(AppConfig::instance().selectedGame(),
                                        QStringLiteral("bot_join_team"),
                                        teamValue);
    emit settingChanged();
}
