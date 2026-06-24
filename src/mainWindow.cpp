#include "mainWindow.h"

#include <QAbstractButton>
#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QWindow>

#include "appConfig.h"
#include "pages/appSettingsPage.h"
#include "pages/botsPage.h"
#include "pages/mapsPage.h"
#include "pages/serverControlsPage.h"
#include "pages/serverPage.h"
#include "pages/serverSettingsPage.h"
#include "serverManager.h"

namespace
{
constexpr QColor NAV_ACCENT_COLOR { 0xc8, 0xa8, 0x00 };

// Renders an SVG resource in two colors — one for the normal state
// and one for the checked/active state — and bundles them into a QIcon
// so Qt switches automatically based on the button's checked state.
QIcon svgTwoStateIcon(const QString& resource, const QColor& normal,
                      const QColor& active, const int size)
{
    auto render = [&resource, size](const QColor& color) -> QPixmap
    {
        QFile file(resource);
        if (file.open(QIODevice::ReadOnly) == false)
        {
            return {};
        }
        QByteArray data = file.readAll();
        data.replace("currentColor", color.name().toLatin1());
        QSvgRenderer renderer(data);
        QPixmap px(size, size);
        px.fill(Qt::transparent);
        QPainter p(&px);
        renderer.render(&p);
        return px;
    };

    QIcon icon;
    icon.addPixmap(render(normal), QIcon::Normal, QIcon::Off);
    icon.addPixmap(render(active), QIcon::Normal, QIcon::On);
    return icon;
}
} // namespace

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    setWindowTitle(QApplication::applicationDisplayName());

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────────
    QFrame* headerBar = new QFrame(this);
    headerBar->setObjectName(QStringLiteral("headerBar"));
    headerBar->setFixedHeight(HEADER_HEIGHT);

    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(HEADER_H_PADDING, HEADER_V_PADDING,
                                     HEADER_H_PADDING, HEADER_V_PADDING);

    m_gameSelector = new QComboBox(headerBar);
    m_gameSelector->setObjectName(QStringLiteral("gameSelector"));
    m_gameSelector->setIconSize(QSize(GAME_ICON_SIZE, GAME_ICON_SIZE));
    m_gameSelector->setMinimumWidth(GAME_SELECTOR_MIN_WIDTH);
    m_gameSelector->addItem(QIcon(QStringLiteral(":/assets/counter-strike-1.6.png")),
                            tr("Counter-Strike 1.6"));
    m_gameSelector->addItem(QIcon(QStringLiteral(":/assets/Condition-Zero.png")),
                            tr("Counter-Strike: Condition Zero"));

    // Restore persisted game selection
    const int savedIndex = (AppConfig::instance().selectedGame() == AppConfig::Game::CZ) ? 1 : 0;
    m_gameSelector->setCurrentIndex(savedIndex);

    headerLayout->addWidget(m_gameSelector);

    headerLayout->addStretch();

    m_startStopBtn = new QPushButton(tr("Start Server"), headerBar);
    m_startStopBtn->setObjectName(QStringLiteral("startStopBtn"));
    m_startStopBtn->setProperty("serverRunning", false);
    headerLayout->addWidget(m_startStopBtn);

    rootLayout->addWidget(headerBar);

    // ── Body ──────────────────────────────────────────────────────────────────
    QWidget* bodyWidget = new QWidget(this);
    QHBoxLayout* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Nav bar
    QWidget* navWidget = new QWidget(bodyWidget);
    navWidget->setObjectName(QStringLiteral("navBar"));
    navWidget->setFixedWidth(NAV_WIDTH);

    QVBoxLayout* navLayout = new QVBoxLayout(navWidget);
    navLayout->setContentsMargins(0, NAV_V_PADDING, 0, NAV_V_PADDING);
    navLayout->setSpacing(0);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    navLayout->addWidget(makeNavButton(tr("Server Overview"), QString(), Page::ServerOverview));

    m_serverControlsNavBtn = makeNavButton(tr("Server Controls"), QString(), Page::ServerControls);
    navLayout->addWidget(m_serverControlsNavBtn);

    navLayout->addWidget(makeNavButton(tr("Maps"),            QString(), Page::Maps));
    navLayout->addWidget(makeNavButton(tr("Bots"),            QString(), Page::Bots));
    navLayout->addWidget(makeNavButton(tr("Server Settings"), QString(), Page::ServerSettings));

    QFrame* navHSep = new QFrame(navWidget);
    navHSep->setFrameShape(QFrame::HLine);
    navHSep->setObjectName(QStringLiteral("navHSep"));
    navLayout->addWidget(navHSep);

    navLayout->addWidget(makeNavButton(tr("App Settings"),
                                       QStringLiteral(":/assets/gear.svg"),
                                       Page::AppSettings));
    navLayout->addStretch();

    bodyLayout->addWidget(navWidget);

    QFrame* vSep = new QFrame(bodyWidget);
    vSep->setFrameShape(QFrame::VLine);
    bodyLayout->addWidget(vSep);

    // Page stack
    m_pageStack = new QStackedWidget(bodyWidget);
    m_serverPage = new ServerPage(this);
    m_pageStack->addWidget(m_serverPage);                 // 0 — ServerOverview

    m_serverControlsPage = new ServerControlsPage(this);
    m_pageStack->addWidget(m_serverControlsPage);         // 1 — ServerControls

    m_pageStack->addWidget(new MapsPage(this));           // 2 — Maps
    m_pageStack->addWidget(new BotsPage(this));           // 3 — Bots
    m_pageStack->addWidget(new ServerSettingsPage(this)); // 4 — ServerSettings
    m_pageStack->addWidget(new AppSettingsPage(this));    // 5 — AppSettings
    bodyLayout->addWidget(m_pageStack, 1);

    rootLayout->addWidget(bodyWidget, 1);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_gameSelector, &QComboBox::currentIndexChanged, this, [this](const int index)
    {
        const AppConfig::Game game = (index == 0) ? AppConfig::Game::CS16 : AppConfig::Game::CZ;
        AppConfig::instance().setSelectedGame(game);
        updateWindowIcon(game);
        m_serverPage->loadForGame(game);
    });

    connect(m_navGroup, &QButtonGroup::idClicked, this, [this](const int id)
    {
        selectPage(static_cast<Page>(id));
    });
    connect(m_startStopBtn, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);

    // ── Server Manager ────────────────────────────────────────────────────────
    m_serverManager = new ServerManager(this);

    connect(m_serverManager, &ServerManager::outputLine,
            m_serverControlsPage, &ServerControlsPage::appendOutput);

    connect(m_serverManager, &ServerManager::stopped, this, [this]()
    {
        setServerRunning(false);
    });

    connect(m_serverControlsPage, &ServerControlsPage::commandSubmitted,
            m_serverManager, &ServerManager::sendCommand);

    selectPage(Page::ServerOverview);
}

QPushButton* MainWindow::makeNavButton(const QString& label,
                                       const QString& iconResource,
                                       const Page page)
{
    QPushButton* btn = new QPushButton(label);
    btn->setObjectName(QStringLiteral("navButton"));
    btn->setCheckable(true);
    btn->setFlat(true);

    if (iconResource.isEmpty() == false)
    {
        const QColor textColor = QGuiApplication::palette().color(QPalette::WindowText);
        btn->setIcon(svgTwoStateIcon(iconResource, textColor, NAV_ACCENT_COLOR, NAV_ICON_SIZE));
        btn->setIconSize(QSize(NAV_ICON_SIZE, NAV_ICON_SIZE));
    }

    m_navGroup->addButton(btn, static_cast<int>(page));
    return btn;
}

void MainWindow::selectPage(const Page page)
{
    m_pageStack->setCurrentIndex(static_cast<int>(page));

    QAbstractButton* btn = m_navGroup->button(static_cast<int>(page));
    if (btn != nullptr)
    {
        btn->setChecked(true);
    }
}

void MainWindow::setServerRunning(const bool running)
{
    m_serverRunning = running;
    m_startStopBtn->setText(running ? tr("Stop Server") : tr("Start Server"));
    m_startStopBtn->setProperty("serverRunning", running);
    m_startStopBtn->style()->unpolish(m_startStopBtn);
    m_startStopBtn->style()->polish(m_startStopBtn);
    m_serverControlsPage->setServerRunning(running);
}

void MainWindow::onStartStopClicked()
{
    if (m_serverRunning == false)
    {
        const AppConfig::Game game = AppConfig::instance().selectedGame();
        const QString serverPath = (game == AppConfig::Game::CZ)
            ? AppConfig::instance().czServerPath()
            : AppConfig::instance().cs16ServerPath();
        const QString gameName = (game == AppConfig::Game::CZ)
            ? QStringLiteral("czero")
            : QStringLiteral("cstrike");

        const bool ok = m_serverManager->start(
            serverPath, gameName,
            m_serverPage->currentIp(),
            m_serverPage->currentPort(),
            m_serverPage->currentMap(),
            m_serverPage->maxPlayers());

        if (ok)
        {
            setServerRunning(true);
        }
    }
    else
    {
        m_serverManager->stop();
        // UI reverts when the stopped() signal arrives from ServerManager.
    }
}

void MainWindow::syncGameSelection()
{
    const int index = (AppConfig::instance().selectedGame() == AppConfig::Game::CZ) ? 1 : 0;
    if (m_gameSelector->currentIndex() != index)
    {
        // setCurrentIndex triggers currentIndexChanged → loadForGame + icon update
        m_gameSelector->setCurrentIndex(index);
    }
    else
    {
        // Index unchanged — setCurrentIndex would be a no-op, so call loadForGame
        // explicitly. Needed after first-time setup when the path just became valid.
        m_serverPage->loadForGame(AppConfig::instance().selectedGame());
    }
}

void MainWindow::updateWindowIcon(const AppConfig::Game game)
{
    const QString resource = (game == AppConfig::Game::CZ)
        ? QStringLiteral(":/assets/app-icon-cz.svg")
        : QStringLiteral(":/assets/app-icon.svg");
    const QIcon icon(resource);
    // Update both the application-level default (taskbar) and this window's
    // title bar icon. Both calls are needed on Wayland for the change to take
    // effect on an already-visible window.
    QApplication::setWindowIcon(icon);
    setWindowIcon(icon);
    if (windowHandle() != nullptr)
    {
        windowHandle()->setIcon(icon);
    }
}
