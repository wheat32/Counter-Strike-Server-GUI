#include "debugPage.h"

#include "../appConfig.h"
#include "../dialogs/errorDetailsDialog.h"
#include "../dialogs/updateAvailableDialog.h"
#include "../dialogs/whatsNewDialog.h"

#include <QDesktopServices>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>
#include <functional>

namespace
{
constexpr int DEBUG_TITLE_FONT_SIZE = 14;
constexpr int DEBUG_H_MARGIN        = 24;
constexpr int DEBUG_V_MARGIN        = 20;
constexpr int DEBUG_LAYOUT_SPACING  = 12;
constexpr int CLEAR_BTN_WIDTH       = 160;
constexpr int CLEAR_ROW_SPACING     = 12;
constexpr int GRID_V_MARGIN         = 4;
constexpr int GRID_H_SPACING        = 12;
constexpr int GRID_V_SPACING        = 6;
constexpr int KEY_LABEL_MIN_WIDTH   = 200;
constexpr int RESET_BTN_WIDTH       = 64;
constexpr int VALUE_COL_STRETCH     = 1;
constexpr int DEFAULT_PORT          = 27015;
constexpr int DEFAULT_MAX_PLAYERS   = 20;
constexpr int UPDATE_CHECK_TIMEOUT_MS = 10000;
const char UPDATE_VERSION_URL[] =
    "https://raw.githubusercontent.com/wheat32/Counter-Strike-Server-GUI/main/src/version.json";

QLabel* makeSectionHeader(const QString& text, QWidget* parent)
{
    QLabel* lbl = new QLabel(text, parent);
    QFont f = lbl->font();
    f.setBold(true);
    lbl->setFont(f);
    return lbl;
}

QFrame* makeDivider(QWidget* parent)
{
    QFrame* line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName(QStringLiteral("sidebarDivider"));
    return line;
}

QString boolStr(const bool v)
{
    return v ? QStringLiteral("true") : QStringLiteral("false");
}

QString themeStr(const AppConfig::Theme t)
{
    switch (t)
    {
        case AppConfig::Theme::Dark:  return QStringLiteral("Dark");
        case AppConfig::Theme::Light: return QStringLiteral("Light");
        default:                      return QStringLiteral("System");
    }
}

QString gameStr(const AppConfig::Game g)
{
    return (g == AppConfig::Game::CZ) ? QStringLiteral("CS:CZ") : QStringLiteral("CS 1.6");
}

QString orEmpty(const QString& s)
{
    return s.isEmpty() ? QStringLiteral("(empty)") : s;
}
} // namespace

DebugPage::DebugPage(QWidget* parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scroll);

    QWidget* content = new QWidget(scroll);
    scroll->setWidget(content);

    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(DEBUG_LAYOUT_SPACING);
    layout->setContentsMargins(DEBUG_H_MARGIN, DEBUG_V_MARGIN, DEBUG_H_MARGIN, DEBUG_V_MARGIN);

    // Page header
    QLabel* titleLabel = new QLabel(tr("Debug Tools"), content);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(DEBUG_TITLE_FONT_SIZE);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel(
        tr("This page is only available in debug builds and is not compiled in release mode."),
        content);
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setStyleSheet(QStringLiteral("color: #888;"));
    layout->addWidget(subtitleLabel);

    layout->addWidget(makeDivider(content));

    // Dialogs section
    layout->addWidget(makeSectionHeader(tr("Dialogs"), content));

    QPushButton* whatsNewBtn = new QPushButton(tr("Test “What’s New” Dialog"), content);
    whatsNewBtn->setObjectName(QStringLiteral("secondaryButton"));
    whatsNewBtn->setCursor(Qt::PointingHandCursor);
    connect(whatsNewBtn, &QPushButton::clicked, this, [this]()
    {
        QString version = QStringLiteral("?.?.?");
        QFile vf(QStringLiteral(":/version.json"));
        if (vf.open(QIODevice::ReadOnly))
        {
            const QJsonObject obj = QJsonDocument::fromJson(vf.readAll()).object();
            vf.close();
            version = obj.value(QStringLiteral("app_version")).toString(version);
        }
        WhatsNewDialog* dlg = new WhatsNewDialog(version, this);
        dlg->setModal(true);
        dlg->show();
    });
    layout->addWidget(whatsNewBtn, 0, Qt::AlignLeft);

    QPushButton* updateDialogBtn = new QPushButton(tr("Test “Update Available” Dialog"), content);
    updateDialogBtn->setObjectName(QStringLiteral("secondaryButton"));
    updateDialogBtn->setCursor(Qt::PointingHandCursor);
    connect(updateDialogBtn, &QPushButton::clicked, this, [this]()
    {
        QString localVersion = QStringLiteral("?.?.?");
        QFile vf(QStringLiteral(":/version.json"));
        if (vf.open(QIODevice::ReadOnly))
        {
            const QJsonObject obj = QJsonDocument::fromJson(vf.readAll()).object();
            vf.close();
            localVersion = obj.value(QStringLiteral("app_version")).toString(localVersion);
        }

        QNetworkRequest request(QUrl(QString::fromLatin1(UPDATE_VERSION_URL)));
        request.setTransferTimeout(UPDATE_CHECK_TIMEOUT_MS);
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, localVersion]()
        {
            reply->deleteLater();
            QString remoteVersion = QStringLiteral("?.?.?");
            if (reply->error() == QNetworkReply::NoError)
            {
                const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
                remoteVersion = obj.value(QStringLiteral("app_version")).toString(remoteVersion);
            }
            UpdateAvailableDialog* dlg = new UpdateAvailableDialog(localVersion, remoteVersion, this);
            dlg->setModal(true);
            dlg->show();
        });
    });
    layout->addWidget(updateDialogBtn, 0, Qt::AlignLeft);

    {
        QString installedVersion = QStringLiteral("?.?.?");
        QFile vf(QStringLiteral(":/version.json"));
        if (vf.open(QIODevice::ReadOnly))
        {
            installedVersion = QJsonDocument::fromJson(vf.readAll()).object()
                                   .value(QStringLiteral("app_version")).toString(installedVersion);
            vf.close();
        }
        const QString releaseUrl =
            QStringLiteral("https://github.com/wheat32/Counter-Strike-Server-GUI/releases/tag/v")
            + installedVersion;

        QHBoxLayout* openRow = new QHBoxLayout();
        openRow->setSpacing(GRID_H_SPACING);

        QPushButton* openReleaseBtn = new QPushButton(tr("Open Installed Release Page"), content);
        openReleaseBtn->setObjectName(QStringLiteral("secondaryButton"));
        openReleaseBtn->setCursor(Qt::PointingHandCursor);
        connect(openReleaseBtn, &QPushButton::clicked, this, [releaseUrl]()
        {
            QDesktopServices::openUrl(QUrl(releaseUrl));
        });
        openRow->addWidget(openReleaseBtn);

        QLabel* urlLabel = new QLabel(releaseUrl);
        urlLabel->setStyleSheet(QStringLiteral("font-family: monospace; color: #888;"));
        urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        urlLabel->adjustSize();

        QScrollArea* urlScroll = new QScrollArea(content);
        urlScroll->setWidget(urlLabel);
        urlScroll->setWidgetResizable(false);
        urlScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        urlScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        urlScroll->setFrameShape(QFrame::NoFrame);
        urlScroll->setFixedHeight(urlLabel->sizeHint().height() + 8);
        openRow->addWidget(urlScroll, 1);

        layout->addLayout(openRow);
    }

    QPushButton* errorDialogBtn = new QPushButton(tr("Test Error Details Dialog (overflow)"), content);
    errorDialogBtn->setObjectName(QStringLiteral("secondaryButton"));
    errorDialogBtn->setCursor(Qt::PointingHandCursor);
    connect(errorDialogBtn, &QPushButton::clicked, this, [this]()
    {
        const QString longLine = tr(
            "This is a very long error line intended to trigger horizontal scrollbar / wrapping "
            "behaviour inside the ErrorDetailsDialog — it just keeps going and going without any "
            "newline break whatsoever. Sample HLDS output: Couldn't allocate dedicated server UDP "
            "port, tried 27015 through 27025. Please check firewall rules and port availability.");

        constexpr int BODY_LINES = 38;
        QStringList lines;
        lines.reserve(BODY_LINES + 2);
        lines << longLine;
        for (int i = 1; i <= BODY_LINES; ++i)
            lines << tr("[line %1] HLDS: Warning — could not load map file maps/de_dust%1.bsp").arg(i);
        lines << longLine;

        ErrorDetailsDialog* dlg = new ErrorDetailsDialog(lines.join(QLatin1Char('\n')), this);
        dlg->setModal(true);
        dlg->show();
    });
    layout->addWidget(errorDialogBtn, 0, Qt::AlignLeft);

    layout->addWidget(makeDivider(content));

    // Settings section
    layout->addWidget(makeSectionHeader(tr("Settings"), content));

    QHBoxLayout* clearRow = new QHBoxLayout();
    clearRow->setSpacing(CLEAR_ROW_SPACING);

    QLabel* clearDesc = new QLabel(
        tr("Delete the config file and reset every value to its default."), content);
    clearDesc->setWordWrap(true);
    clearDesc->setStyleSheet(QStringLiteral("color: #888;"));
    clearRow->addWidget(clearDesc, 1);

    QPushButton* clearAllBtn = new QPushButton(tr("Clear All Settings"), content);
    clearAllBtn->setObjectName(QStringLiteral("dangerButton"));
    clearAllBtn->setCursor(Qt::PointingHandCursor);
    clearAllBtn->setFixedWidth(CLEAR_BTN_WIDTH);
    connect(clearAllBtn, &QPushButton::clicked, this, [this]()
    {
        QMessageBox mb(this);
        mb.setWindowTitle(tr("Clear All Settings"));
        mb.setText(tr("This will delete the config file and reset every setting to its default. "
                      "The change takes effect immediately in memory."));
        mb.setIcon(QMessageBox::Warning);
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        mb.setDefaultButton(QMessageBox::Cancel);
        if (mb.exec() == QMessageBox::Yes)
        {
            AppConfig::instance().resetToDefaults();
            refreshValues();
        }
    });
    clearRow->addWidget(clearAllBtn, 0, Qt::AlignRight);
    layout->addLayout(clearRow);

    // Per-key settings grid
    QWidget* gridWidget = new QWidget(content);
    QGridLayout* grid = new QGridLayout(gridWidget);
    grid->setContentsMargins(0, GRID_V_MARGIN, 0, GRID_V_MARGIN);
    grid->setHorizontalSpacing(GRID_H_SPACING);
    grid->setVerticalSpacing(GRID_V_SPACING);
    grid->setColumnStretch(1, VALUE_COL_STRETCH);

    int row = 0;
    auto addRow = [&](const QString& key,
                      const QString& defaultHint,
                      std::function<void()> onReset) -> QLabel*
    {
        QLabel* keyLbl = new QLabel(
            QStringLiteral("<b>%1</b>").arg(key.toHtmlEscaped()), gridWidget);
        keyLbl->setTextFormat(Qt::RichText);
        keyLbl->setMinimumWidth(KEY_LABEL_MIN_WIDTH);

        QLabel* valLbl = new QLabel(gridWidget);
        valLbl->setTextFormat(Qt::PlainText);
        valLbl->setStyleSheet(QStringLiteral("font-family: monospace;"));

        QPushButton* resetBtn = new QPushButton(tr("Reset"), gridWidget);
        resetBtn->setObjectName(QStringLiteral("secondaryButton"));
        resetBtn->setCursor(Qt::PointingHandCursor);
        resetBtn->setToolTip(tr("Reset to default: %1").arg(defaultHint));
        resetBtn->setFixedWidth(RESET_BTN_WIDTH);
        connect(resetBtn, &QPushButton::clicked, this, [this, onReset]()
        {
            onReset();
            refreshValues();
        });

        grid->addWidget(keyLbl,   row, 0);
        grid->addWidget(valLbl,   row, 1);
        grid->addWidget(resetBtn, row, 2);
        ++row;
        return valLbl;
    };

    // General
    m_valTheme = addRow(
        QStringLiteral("theme"), QStringLiteral("System"),
        []() { AppConfig::instance().setTheme(AppConfig::Theme::System); });

    m_valSelectedGame = addRow(
        QStringLiteral("selected_game"), QStringLiteral("CS:CZ"),
        []() { AppConfig::instance().setSelectedGame(AppConfig::Game::CZ); });

    m_valCheckForUpdates = addRow(
        QStringLiteral("check_for_updates"), QStringLiteral("true"),
        []() { AppConfig::instance().setCheckForUpdates(true); });

    m_valLastSeenVersion = addRow(
        QStringLiteral("last_seen_version"), QStringLiteral("(empty)"),
        []() { AppConfig::instance().setLastSeenVersion(QString()); });

    // CS 1.6
    m_valCs16Path = addRow(
        QStringLiteral("cs16_server_path"), QStringLiteral("(empty)"),
        []() { AppConfig::instance().setCs16ServerPath(QString()); });

    m_valCs16Ip = addRow(
        QStringLiteral("cs16_ip"), QStringLiteral("127.0.0.1"),
        []() { AppConfig::instance().setCs16Ip(QStringLiteral("127.0.0.1")); });

    m_valCs16Port = addRow(
        QStringLiteral("cs16_port"), QStringLiteral("27015"),
        []() { AppConfig::instance().setCs16Port(DEFAULT_PORT); });

    m_valCs16StartMap = addRow(
        QStringLiteral("cs16_start_map"), QStringLiteral("(empty)"),
        []() { AppConfig::instance().setCs16StartMap(QString()); });

    m_valCs16MaxPlayers = addRow(
        QStringLiteral("cs16_max_players"), QStringLiteral("20"),
        []() { AppConfig::instance().setCs16MaxPlayers(DEFAULT_MAX_PLAYERS); });

    // CS:CZ
    m_valCzPath = addRow(
        QStringLiteral("cz_server_path"), QStringLiteral("(empty)"),
        []() { AppConfig::instance().setCzServerPath(QString()); });

    m_valCzIp = addRow(
        QStringLiteral("cz_ip"), QStringLiteral("127.0.0.1"),
        []() { AppConfig::instance().setCzIp(QStringLiteral("127.0.0.1")); });

    m_valCzPort = addRow(
        QStringLiteral("cz_port"), QStringLiteral("27015"),
        []() { AppConfig::instance().setCzPort(DEFAULT_PORT); });

    m_valCzStartMap = addRow(
        QStringLiteral("cz_start_map"), QStringLiteral("(empty)"),
        []() { AppConfig::instance().setCzStartMap(QString()); });

    m_valCzMaxPlayers = addRow(
        QStringLiteral("cz_max_players"), QStringLiteral("20"),
        []() { AppConfig::instance().setCzMaxPlayers(DEFAULT_MAX_PLAYERS); });

    layout->addWidget(gridWidget);
    layout->addStretch();

    refreshValues();
}

void DebugPage::refreshValues() const
{
    const AppConfig& cfg = AppConfig::instance();

    m_valTheme->setText(themeStr(cfg.theme()));
    m_valSelectedGame->setText(gameStr(cfg.selectedGame()));
    m_valCheckForUpdates->setText(boolStr(cfg.checkForUpdates()));
    m_valLastSeenVersion->setText(orEmpty(cfg.lastSeenVersion()));

    m_valCs16Path->setText(orEmpty(cfg.cs16ServerPath()));
    m_valCs16Ip->setText(cfg.cs16Ip());
    m_valCs16Port->setText(QString::number(cfg.cs16Port()));
    m_valCs16StartMap->setText(orEmpty(cfg.cs16StartMap()));
    m_valCs16MaxPlayers->setText(QString::number(cfg.cs16MaxPlayers()));

    m_valCzPath->setText(orEmpty(cfg.czServerPath()));
    m_valCzIp->setText(cfg.czIp());
    m_valCzPort->setText(QString::number(cfg.czPort()));
    m_valCzStartMap->setText(orEmpty(cfg.czStartMap()));
    m_valCzMaxPlayers->setText(QString::number(cfg.czMaxPlayers()));
}
