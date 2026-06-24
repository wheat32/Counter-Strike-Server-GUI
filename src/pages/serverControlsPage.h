#pragma once

#include <QWidget>

class QFrame;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class ServerControlsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ServerControlsPage(QWidget* parent = nullptr);

    // Appends a line to the output area. Called by ServerManager as output arrives.
    void appendOutput(const QString& line);

    // Updates the "server not running" warning and enables/disables controls.
    void setServerRunning(bool running);

signals:
    // Emitted when the user submits a command. Connect to ServerManager::sendCommand.
    void commandSubmitted(const QString& cmd);

private:
    QFrame*         m_notRunningBanner = nullptr;
    QWidget*        m_controls         = nullptr;  // groups + input — disabled when not running
    QPlainTextEdit* m_outputArea       = nullptr;
    QLineEdit*      m_cmdInput         = nullptr;

    QPushButton* makeQuickBtn(const QString& label, const QString& cmd, QWidget* parent);
    void fillCommand(const QString& cmd);
    void submitCommand();
};
