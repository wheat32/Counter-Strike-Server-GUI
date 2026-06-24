#pragma once

#include <QDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "firewallChecker.h"

// Simple dialog showing copy-pasteable commands to open a port in the system firewall.
class FirewallHelpDialog : public QDialog
{
    Q_OBJECT

    static constexpr int DIALOG_MIN_WIDTH      = 500;
    static constexpr int DIALOG_DEFAULT_HEIGHT = 320;
    static constexpr int CMDS_MIN_HEIGHT       = 100;
    static constexpr int MARGIN                = 20;
    static constexpr int SPACING               = 12;

public:
    explicit FirewallHelpDialog(int port,
                                FirewallChecker::FirewallType type,
                                QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(tr("Opening Port %1 in Your Firewall").arg(port));
        setMinimumWidth(DIALOG_MIN_WIDTH);
        resize(DIALOG_MIN_WIDTH, DIALOG_DEFAULT_HEIGHT);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
        layout->setSpacing(SPACING);

        QString firewallName;
        QString commands;

        if (type == FirewallChecker::FirewallType::Firewalld)
        {
            firewallName = QStringLiteral("firewalld");
            commands = QStringLiteral(
                "sudo firewall-cmd --permanent --add-port=%1/tcp\n"
                "sudo firewall-cmd --permanent --add-port=%1/udp\n"
                "sudo firewall-cmd --reload").arg(port);
        }
        else
        {
            firewallName = QStringLiteral("ufw");
            commands = QStringLiteral(
                "sudo ufw allow %1/tcp\n"
                "sudo ufw allow %1/udp").arg(port);
        }

        QLabel* descLabel = new QLabel(
            tr("Your system uses <b>%1</b>. Run the following commands in a terminal "
               "to allow incoming traffic on port %2 for both TCP and UDP:")
                .arg(firewallName).arg(port), this);
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel);

        QPlainTextEdit* cmdEdit = new QPlainTextEdit(commands, this);
        cmdEdit->setObjectName(QStringLiteral("helpCmds"));
        cmdEdit->setReadOnly(true);
        cmdEdit->setMinimumHeight(CMDS_MIN_HEIGHT);
        layout->addWidget(cmdEdit, 1);

        QLabel* noteLabel = new QLabel(
            tr("After running these commands, the firewall status on this page will "
               "update automatically within a few seconds."), this);
        noteLabel->setWordWrap(true);
        noteLabel->setObjectName(QStringLiteral("helpNote"));
        layout->addWidget(noteLabel);

        QPushButton* closeBtn = new QPushButton(tr("Close"), this);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);
    }
};
