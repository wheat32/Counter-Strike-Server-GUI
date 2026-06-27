#pragma once

#include <QLabel>
#include <QNetworkAccessManager>
#include <QWidget>

// DebugPage – only compiled in debug builds (QT_DEBUG).
// Provides developer-facing tools: triggering dialogs manually and
// inspecting / resetting saved application settings.
class DebugPage : public QWidget
{
    Q_OBJECT

public:
    explicit DebugPage(QWidget* parent = nullptr);

private slots:
    void refreshValues() const;

private:
    QLabel* m_valTheme           = nullptr;
    QLabel* m_valSelectedGame    = nullptr;
    QLabel* m_valCheckForUpdates = nullptr;
    QLabel* m_valLastSeenVersion = nullptr;

    QLabel* m_valCs16Path       = nullptr;
    QLabel* m_valCs16Ip         = nullptr;
    QLabel* m_valCs16Port       = nullptr;
    QLabel* m_valCs16StartMap   = nullptr;
    QLabel* m_valCs16MaxPlayers = nullptr;

    QLabel* m_valCzPath         = nullptr;
    QLabel* m_valCzIp           = nullptr;
    QLabel* m_valCzPort         = nullptr;
    QLabel* m_valCzStartMap     = nullptr;
    QLabel* m_valCzMaxPlayers   = nullptr;

    QNetworkAccessManager* m_networkManager = nullptr;
};
