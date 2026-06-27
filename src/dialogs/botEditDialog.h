#pragma once

#include <QDialog>

#include "../serverFiles.h"

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class NumberSpinner;

// Dialog for adding or editing a single bot profile in BotProfile.db.
class BotEditDialog : public QDialog
{
    Q_OBJECT

    static constexpr int DIALOG_MIN_WIDTH = 400;
    static constexpr int NAME_MAX_LEN     = 31;

public:
    explicit BotEditDialog(QWidget* parent = nullptr);
    explicit BotEditDialog(const ServerFiles::BotProfile& profile, QWidget* parent = nullptr);

    [[nodiscard]] ServerFiles::BotProfile profile() const;

private:
    void setup();
    void updateNameCounter() const;
    void updateOkEnabled() const;

    QLineEdit*    m_nameEdit      = nullptr;
    QLabel*       m_nameCounter   = nullptr;
    QComboBox*    m_skillCombo    = nullptr;
    QComboBox*    m_weaponCombo   = nullptr;
    QComboBox*    m_skinCombo     = nullptr;
    NumberSpinner* m_pitchSpinner = nullptr;
    QPushButton*  m_okBtn         = nullptr;
};
