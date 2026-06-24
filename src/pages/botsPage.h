#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class BotsPage : public QWidget
{
    Q_OBJECT
public:
    explicit BotsPage(QWidget* parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QLabel* titleLabel = new QLabel(tr("Bots"), this);
        titleLabel->setObjectName(QStringLiteral("pageTitle"));
        layout->addWidget(titleLabel);

        QLabel* placeholder = new QLabel(tr("Bot configuration — coming soon"), this);
        placeholder->setObjectName(QStringLiteral("placeholderLabel"));
        placeholder->setAlignment(Qt::AlignCenter);
        layout->addWidget(placeholder, 1);
    }
};
