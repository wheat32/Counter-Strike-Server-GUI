#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class MapsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MapsPage(QWidget* parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QLabel* titleLabel = new QLabel(tr("Maps"), this);
        titleLabel->setObjectName(QStringLiteral("pageTitle"));
        layout->addWidget(titleLabel);

        QLabel* placeholder = new QLabel(tr("Map browser — coming soon"), this);
        placeholder->setObjectName(QStringLiteral("placeholderLabel"));
        placeholder->setAlignment(Qt::AlignCenter);
        layout->addWidget(placeholder, 1);
    }
};
