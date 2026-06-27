#include "gameServerTypeDialog.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

GameServerTypeDialog::GameServerTypeDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Detect Game Server"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    setMinimumWidth(DIALOG_MIN_WIDTH);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN);
    layout->setSpacing(DIALOG_SPACING);

    QLabel* heading = new QLabel(
        QStringLiteral("<b>%1</b>").arg(tr("Server Installation Detected").toHtmlEscaped()),
        this);
    heading->setTextFormat(Qt::RichText);
    layout->addWidget(heading);

    QLabel* body = new QLabel(
        tr("CS Server Manager has detected a HLDS server installation in the same "
           "directory as this AppImage.\n\nWhich game server is installed here?"),
        this);
    body->setWordWrap(true);
    layout->addWidget(body);

    layout->addStretch();

    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    QPushButton* czBtn   = new QPushButton(tr("Counter-Strike: Condition Zero"), this);
    QPushButton* cs16Btn = new QPushButton(tr("Counter-Strike 1.6"), this);

    btnRow->addStretch();
    btnRow->addWidget(cs16Btn);
    btnRow->addWidget(czBtn);
    layout->addLayout(btnRow);

    connect(czBtn, &QPushButton::clicked, this, [this]
    {
        m_selectedGame = AppConfig::Game::CZ;
        accept();
    });
    connect(cs16Btn, &QPushButton::clicked, this, [this]
    {
        m_selectedGame = AppConfig::Game::CS16;
        accept();
    });
}

void GameServerTypeDialog::closeEvent(QCloseEvent* event)
{
    event->ignore();
}

void GameServerTypeDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        event->ignore();
        return;
    }
    QDialog::keyPressEvent(event);
}
