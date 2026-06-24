#include "serverSetupDialog.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QVBoxLayout>

#include "appConfig.h"
#include "serverUtils.h"

namespace
{
QString gameDisplayName(const AppConfig::Game game)
{
    return (game == AppConfig::Game::CZ)
        ? QObject::tr("Counter-Strike: Condition Zero")
        : QObject::tr("Counter-Strike 1.6");
}
} // namespace

ServerSetupDialog::ServerSetupDialog(const AppConfig::Game game, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint)
    , m_game(game)
    , m_otherGame(game == AppConfig::Game::CZ ? AppConfig::Game::CS16 : AppConfig::Game::CZ)
{
    const QString otherPath = (m_otherGame == AppConfig::Game::CZ)
        ? AppConfig::instance().czServerPath()
        : AppConfig::instance().cs16ServerPath();
    m_otherGameValid = isValidServerPath(otherPath);

    // When both games are unconfigured the user picks which to set up first.
    const bool bothInvalid = (m_otherGameValid == false);

    setWindowTitle(tr("Server Location Required"));
    setMinimumWidth(DIALOG_MIN_WIDTH);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN);
    layout->setSpacing(DIALOG_SPACING);

    // ── Heading ───────────────────────────────────────────────────────────────
    QLabel* headingLabel = new QLabel(
        bothInvalid
            ? tr("No server installations are configured yet.")
            : tr("HLDS installation not found for %1.").arg(gameDisplayName(m_game)),
        this);
    headingLabel->setObjectName(QStringLiteral("setupDialogHeading"));
    headingLabel->setWordWrap(true);
    layout->addWidget(headingLabel);

    QLabel* descLabel = new QLabel(
        bothInvalid
            ? tr("Select which game you want to configure first, then locate its HLDS "
                 "installation directory — the folder containing <b>hlds_run</b>.")
            : tr("Please locate your HLDS installation directory — the folder that contains "
                 "the <b>hlds_run</b> file."),
        this);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    // ── Game selector (first-run only) ────────────────────────────────────────
    if (bothInvalid)
    {
        m_gameCombo = new QComboBox(this);
        m_gameCombo->addItem(tr("Counter-Strike 1.6"));
        m_gameCombo->addItem(tr("Counter-Strike: Condition Zero"));
        m_gameCombo->setCurrentIndex((m_game == AppConfig::Game::CZ) ? 1 : 0);
        layout->addWidget(m_gameCombo);
    }

    // ── Path row ──────────────────────────────────────────────────────────────
    QHBoxLayout* pathRow = new QHBoxLayout();
    pathRow->setSpacing(PATH_ROW_SPACING);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(tr("/path/to/hlds"));

    const QString existing = (m_game == AppConfig::Game::CZ)
        ? AppConfig::instance().czServerPath()
        : AppConfig::instance().cs16ServerPath();
    if (existing.isEmpty() == false)
    {
        m_pathEdit->setText(existing);
    }

    QPushButton* browseBtn = new QPushButton(tr("Browse…"), this);
    browseBtn->setMaximumWidth(BROWSE_BTN_MAX_W);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(browseBtn);
    layout->addLayout(pathRow);

    // ── Status label ──────────────────────────────────────────────────────────
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("setupStatusLabel"));
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    // ── Button row ────────────────────────────────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(BTN_ROW_SPACING);

    const QString cancelText = m_otherGameValid
        ? tr("Switch to %1").arg(gameDisplayName(m_otherGame))
        : tr("Quit");
    QPushButton* cancelBtn = new QPushButton(cancelText, this);

    m_locateBtn = new QPushButton(tr("Use This Location"), this);
    m_locateBtn->setProperty("accent", true);
    m_locateBtn->setEnabled(false);

    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_locateBtn);
    layout->addLayout(btnRow);

    // ── Connections ───────────────────────────────────────────────────────────
    if (m_gameCombo != nullptr)
    {
        connect(m_gameCombo, &QComboBox::currentIndexChanged,
                this, &ServerSetupDialog::onGameComboChanged);
    }
    connect(browseBtn,  &QPushButton::clicked,   this, &ServerSetupDialog::onBrowseClicked);
    connect(m_locateBtn,&QPushButton::clicked,   this, &ServerSetupDialog::onLocateClicked);
    connect(cancelBtn,  &QPushButton::clicked,   this, &ServerSetupDialog::onCancelClicked);
    connect(m_pathEdit, &QLineEdit::textChanged, this, &ServerSetupDialog::onPathChanged);

    if (existing.isEmpty() == false)
    {
        onPathChanged(existing);
    }
}

void ServerSetupDialog::closeEvent(QCloseEvent* event)
{
    event->ignore();
    onCancelClicked();
}

void ServerSetupDialog::onGameComboChanged(const int index)
{
    m_game = (index == 0) ? AppConfig::Game::CS16 : AppConfig::Game::CZ;

    const QString existingPath = (m_game == AppConfig::Game::CZ)
        ? AppConfig::instance().czServerPath()
        : AppConfig::instance().cs16ServerPath();

    // Block textChanged so we call onPathChanged exactly once below.
    {
        const QSignalBlocker blocker(m_pathEdit);
        m_pathEdit->setText(existingPath);
    }
    onPathChanged(existingPath);
}

void ServerSetupDialog::onBrowseClicked()
{
    const QString startDir = m_pathEdit->text().isEmpty()
        ? QDir::homePath()
        : m_pathEdit->text();

    const QString chosen = QFileDialog::getExistingDirectory(
        this, tr("Select HLDS Installation Directory"), startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (chosen.isEmpty() == false)
    {
        m_pathEdit->setText(chosen);
    }
}

void ServerSetupDialog::onPathChanged(const QString& text)
{
    const bool valid = isValidServerPath(text);
    m_locateBtn->setEnabled(valid);

    if (text.isEmpty())
    {
        m_statusLabel->setText(QString());
        m_statusLabel->setProperty("status", QVariant());
    }
    else if (valid)
    {
        m_statusLabel->setText(tr("✓  hlds_run found."));
        m_statusLabel->setProperty("status", QStringLiteral("ok"));
    }
    else
    {
        m_statusLabel->setText(tr("✗  hlds_run not found in this directory."));
        m_statusLabel->setProperty("status", QStringLiteral("error"));
    }

    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

void ServerSetupDialog::onLocateClicked()
{
    const QString path = m_pathEdit->text();
    if (isValidServerPath(path) == false)
        return;

    if (m_game == AppConfig::Game::CZ)
    {
        AppConfig::instance().setCzServerPath(path);
    }
    else
    {
        AppConfig::instance().setCs16ServerPath(path);
    }
    // Switch the selected game to match what the user just configured.
    // In the single-invalid case this is a no-op; in the both-invalid case
    // it switches to whichever game the user chose in the dropdown.
    AppConfig::instance().setSelectedGame(m_game);
    accept();
}

void ServerSetupDialog::onCancelClicked()
{
    if (m_otherGameValid)
    {
        AppConfig::instance().setSelectedGame(m_otherGame);
        reject();
    }
    else
    {
        // QApplication::quit() cannot exit a QDialog::exec() loop — Qt 6 uses
        // QEventLoop::DialogExec which intentionally ignores application quit
        // signals. Instead, set a flag and call reject() to exit the dialog's
        // local event loop normally; main.cpp checks the flag and returns 0.
        m_userWantsToQuit = true;
        reject();
    }
}
