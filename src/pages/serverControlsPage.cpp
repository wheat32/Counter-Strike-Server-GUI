#include "serverControlsPage.h"

#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

namespace
{
constexpr int PAGE_MARGIN       = 16;
constexpr int SECTION_SPACING   = 10;
constexpr int BANNER_MARGIN     = 12;
constexpr int BANNER_SPACING    = 8;
constexpr int QUICK_BTN_SPACING = 6;
constexpr int CMD_ROW_SPACING   = 6;
constexpr int OUTPUT_MIN_HEIGHT = 180;
constexpr int SEND_BTN_MAX_W    = 80;
} // namespace

ServerControlsPage::ServerControlsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel(tr("Server Controls"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    outerLayout->addWidget(titleLabel);

    QWidget* content = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN);
    layout->setSpacing(SECTION_SPACING);

    // ── "Server not running" warning banner ───────────────────────────────────
    m_notRunningBanner = new QFrame(content);
    m_notRunningBanner->setObjectName(QStringLiteral("serverNotRunningBanner"));

    QHBoxLayout* bannerLayout = new QHBoxLayout(m_notRunningBanner);
    bannerLayout->setContentsMargins(BANNER_MARGIN, BANNER_MARGIN,
                                     BANNER_MARGIN, BANNER_MARGIN);
    bannerLayout->setSpacing(BANNER_SPACING);

    QLabel* bannerIcon = new QLabel(QStringLiteral("⚠"), m_notRunningBanner);
    bannerIcon->setObjectName(QStringLiteral("serverNotRunningBannerIcon"));
    bannerLayout->addWidget(bannerIcon);

    QLabel* bannerText = new QLabel(
        tr("The server is not running. Start it from <b>Server Overview</b> "
           "to use these controls."), m_notRunningBanner);
    bannerText->setWordWrap(true);
    bannerLayout->addWidget(bannerText, 1);

    layout->addWidget(m_notRunningBanner);

    // ── Output area ───────────────────────────────────────────────────────────
    m_outputArea = new QPlainTextEdit(content);
    m_outputArea->setObjectName(QStringLiteral("serverOutputArea"));
    m_outputArea->setReadOnly(true);
    m_outputArea->setMinimumHeight(OUTPUT_MIN_HEIGHT);
    m_outputArea->setPlaceholderText(tr("Server output will appear here…"));
    layout->addWidget(m_outputArea, 1);

    // ── Controls (quick commands + input) — disabled when not running ─────────
    m_controls = new QWidget(content);
    QVBoxLayout* controlsLayout = new QVBoxLayout(m_controls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(SECTION_SPACING);

    // Quick Commands group
    {
        QGroupBox* group = new QGroupBox(tr("Quick Commands"), m_controls);
        QVBoxLayout* groupLayout = new QVBoxLayout(group);
        groupLayout->setSpacing(QUICK_BTN_SPACING);

        QHBoxLayout* row1 = new QHBoxLayout();
        row1->setSpacing(QUICK_BTN_SPACING);
        row1->addWidget(makeQuickBtn(tr("Status"),        QStringLiteral("status"),           group));
        row1->addWidget(makeQuickBtn(tr("Restart Game"),  QStringLiteral("mp_restartgame 1"), group));
        row1->addWidget(makeQuickBtn(tr("Change Level…"), QStringLiteral("changelevel "),     group));
        row1->addStretch();
        groupLayout->addLayout(row1);

        QHBoxLayout* row2 = new QHBoxLayout();
        row2->setSpacing(QUICK_BTN_SPACING);
        row2->addWidget(makeQuickBtn(tr("Add CT Bot"),    QStringLiteral("bot_add_ct"), group));
        row2->addWidget(makeQuickBtn(tr("Add T Bot"),     QStringLiteral("bot_add_t"),  group));
        row2->addWidget(makeQuickBtn(tr("Kick All Bots"), QStringLiteral("bot_kick"),   group));
        row2->addStretch();
        groupLayout->addLayout(row2);

        QHBoxLayout* row3 = new QHBoxLayout();
        row3->setSpacing(QUICK_BTN_SPACING);
        row3->addWidget(makeQuickBtn(tr("Say…"),         QStringLiteral("say "),        group));
        row3->addWidget(makeQuickBtn(tr("Kick Player…"), QStringLiteral("kick "),       group));
        row3->addWidget(makeQuickBtn(tr("Cheats On"),    QStringLiteral("sv_cheats 1"), group));
        row3->addWidget(makeQuickBtn(tr("Cheats Off"),   QStringLiteral("sv_cheats 0"), group));
        row3->addStretch();
        groupLayout->addLayout(row3);

        controlsLayout->addWidget(group);
    }

    // Command input row
    {
        QHBoxLayout* cmdRow = new QHBoxLayout();
        cmdRow->setSpacing(CMD_ROW_SPACING);

        QLabel* promptLabel = new QLabel(QStringLiteral(">"), m_controls);
        promptLabel->setObjectName(QStringLiteral("cmdPromptLabel"));
        cmdRow->addWidget(promptLabel);

        m_cmdInput = new QLineEdit(m_controls);
        m_cmdInput->setObjectName(QStringLiteral("cmdInput"));
        m_cmdInput->setPlaceholderText(tr("Type a command…"));
        cmdRow->addWidget(m_cmdInput, 1);

        QPushButton* sendBtn = new QPushButton(tr("Send"), m_controls);
        sendBtn->setMaximumWidth(SEND_BTN_MAX_W);
        sendBtn->setProperty("accent", true);
        cmdRow->addWidget(sendBtn);

        controlsLayout->addLayout(cmdRow);

        connect(sendBtn,    &QPushButton::clicked,     this, &ServerControlsPage::submitCommand);
        connect(m_cmdInput, &QLineEdit::returnPressed, this, &ServerControlsPage::submitCommand);
    }

    layout->addWidget(m_controls);
    outerLayout->addWidget(content, 1);

    // Initial state: server not running
    setServerRunning(false);
}

void ServerControlsPage::setServerRunning(const bool running)
{
    m_notRunningBanner->setVisible(running == false);
    m_controls->setEnabled(running);
}

QPushButton* ServerControlsPage::makeQuickBtn(const QString& label,
                                               const QString& cmd,
                                               QWidget* parent)
{
    QPushButton* btn = new QPushButton(label, parent);
    connect(btn, &QPushButton::clicked, this, [this, cmd]()
    {
        fillCommand(cmd);
    });
    return btn;
}

void ServerControlsPage::fillCommand(const QString& cmd)
{
    m_cmdInput->setText(cmd);
    m_cmdInput->setFocus();
    m_cmdInput->setCursorPosition(cmd.length());
}

void ServerControlsPage::submitCommand()
{
    const QString cmd = m_cmdInput->text().trimmed();
    if (cmd.isEmpty()) return;
    appendOutput(QStringLiteral("] ") + cmd);
    m_cmdInput->clear();
    emit commandSubmitted(cmd);
}

void ServerControlsPage::appendOutput(const QString& line)
{
    m_outputArea->appendPlainText(line);
    m_outputArea->moveCursor(QTextCursor::End);
    m_outputArea->ensureCursorVisible();
}
