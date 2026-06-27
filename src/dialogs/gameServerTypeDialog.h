#pragma once

#include <QDialog>
#include "appConfig.h"

// Shown on first AppImage launch when hlds_run is detected in the same directory.
// Inescapable — the user must identify which game is installed before continuing.
class GameServerTypeDialog : public QDialog
{
    Q_OBJECT

    static constexpr int DIALOG_MIN_WIDTH = 440;
    static constexpr int DIALOG_MARGIN    = 24;
    static constexpr int DIALOG_SPACING   = 14;

public:
    explicit GameServerTypeDialog(QWidget* parent = nullptr);

    AppConfig::Game selectedGame() const { return m_selectedGame; }

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    AppConfig::Game m_selectedGame = AppConfig::Game::CZ;
};
