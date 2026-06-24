#pragma once

#include <QLineEdit>
#include <QWidget>

class AppSettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AppSettingsPage(QWidget* parent = nullptr);

private:
    QLineEdit* m_cs16PathEdit = nullptr;
    QLineEdit* m_czPathEdit   = nullptr;

    void onBrowse(QLineEdit* pathEdit, bool isCZ);
};
