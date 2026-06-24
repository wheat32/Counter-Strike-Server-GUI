#pragma once

#include <QDialog>

// AboutDialog – shows app version, package type, credits, and acknowledgements.
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
};
