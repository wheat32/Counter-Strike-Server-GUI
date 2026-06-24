#pragma once

#include <QDialog>
#include "appConfig.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

// Modal dialog shown at startup when no valid server path is configured.
//
// Dismissal rules:
//   • "Use This Location" — saves the path for the selected game and accepts.
//   • Cancel button / window close:
//       - If the OTHER game has a valid path → switch to it and close.
//       - Otherwise → quit the application.
//
// Game-selector dropdown:
//   Shown only when BOTH games have no valid path (first-run scenario).
//   The user can choose which game to configure first; the other game will
//   prompt again when the user switches to it in the main window.
class ServerSetupDialog : public QDialog
{
    Q_OBJECT

    static constexpr int DIALOG_MIN_WIDTH = 520;
    static constexpr int DIALOG_MARGIN    = 24;
    static constexpr int DIALOG_SPACING   = 12;
    static constexpr int PATH_ROW_SPACING = 6;
    static constexpr int BTN_ROW_SPACING  = 8;
    static constexpr int BROWSE_BTN_MAX_W = 90;

public:
    explicit ServerSetupDialog(AppConfig::Game game, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    AppConfig::Game m_game;
    AppConfig::Game m_otherGame;
    bool            m_otherGameValid = false;

    QComboBox*   m_gameCombo        = nullptr;  // non-null only in the both-invalid case
    QLineEdit*   m_pathEdit         = nullptr;
    QLabel*      m_statusLabel      = nullptr;
    QPushButton* m_locateBtn        = nullptr;
    bool         m_userWantsToQuit  = false;

    void onGameComboChanged(int index);
    void onBrowseClicked();
    void onPathChanged(const QString& text);
    void onLocateClicked();
    void onCancelClicked();

public:
    // Returns true if the user pressed Quit (rather than switching games).
    // Check this after exec() returns to decide whether to exit the process.
    bool userWantsToQuit() const { return m_userWantsToQuit; }
};
