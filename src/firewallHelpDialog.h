#pragma once

#include <QCheckBox>
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

        const bool isFirewalld = (type == FirewallChecker::FirewallType::Firewalld);
        QString firewallName;
        QString commands;

        if (isFirewalld)
        {
            firewallName = QStringLiteral("firewalld");
            commands = firewalldCommands(port, true);
        }
        else
        {
            // ufw rules are always saved; there is no runtime-only variant.
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

        if (isFirewalld)
        {
            QCheckBox* permanentCheck = new QCheckBox(
                tr("Make permanent (keep the port open after a restart)"), this);
            permanentCheck->setChecked(true);
            connect(permanentCheck, &QCheckBox::toggled, this, [cmdEdit, port](const bool permanent)
            {
                cmdEdit->setPlainText(firewalldCommands(port, permanent));
            });
            layout->addWidget(permanentCheck);
        }

        QLabel* noteLabel = new QLabel(
            tr("After running these commands, close this window and the firewall "
               "status will be checked again."), this);
        noteLabel->setWordWrap(true);
        noteLabel->setObjectName(QStringLiteral("helpNote"));
        layout->addWidget(noteLabel);

        QPushButton* closeBtn = new QPushButton(tr("Close"), this);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);
    }

private:
    // Permanent rules are saved and applied with --reload. Runtime-only rules
    // take effect immediately and are lost on reload or restart, so they must
    // not be followed by --reload (which would discard them straight away).
    static QString firewalldCommands(const int port, const bool permanent)
    {
        if (permanent)
        {
            return QStringLiteral(
                "sudo firewall-cmd --permanent --add-port=%1/tcp\n"
                "sudo firewall-cmd --permanent --add-port=%1/udp\n"
                "sudo firewall-cmd --reload").arg(port);
        }
        return QStringLiteral(
            "sudo firewall-cmd --add-port=%1/tcp\n"
            "sudo firewall-cmd --add-port=%1/udp").arg(port);
    }
};
