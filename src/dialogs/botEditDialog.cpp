#include "botEditDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../widgets/numberSpinner.h"

namespace
{
constexpr int DIALOG_MARGIN   = 20;
constexpr int DIALOG_SPACING  = 12;
constexpr int PITCH_MIN       = 1;
constexpr int PITCH_MAX       = 200;
constexpr int PITCH_DEFAULT   = 100;
constexpr int PITCH_WIDTH     = 80;
} // namespace

BotEditDialog::BotEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setup();
}

BotEditDialog::BotEditDialog(const ServerFiles::BotProfile& profile, QWidget* parent)
    : QDialog(parent)
{
    setup();

    m_nameEdit->setText(profile.name);

    const int skillIdx = ServerFiles::knownSkillTemplates().indexOf(profile.skillTemplate);
    if (skillIdx >= 0) m_skillCombo->setCurrentIndex(skillIdx);

    if (profile.weaponTemplate.isEmpty())
    {
        m_weaponCombo->setCurrentIndex(0); // "None"
    }
    else
    {
        const int wi = ServerFiles::knownWeaponTemplates().indexOf(profile.weaponTemplate);
        if (wi >= 0) m_weaponCombo->setCurrentIndex(wi + 1); // +1 for "None" at index 0
    }

    m_skinCombo->setCurrentIndex(qBound(0, profile.skin, 4));
    m_pitchSpinner->setValue(qBound(PITCH_MIN, profile.voicePitch, PITCH_MAX));
}

void BotEditDialog::setup()
{
    setWindowTitle(tr("Bot Profile"));
    setMinimumWidth(DIALOG_MIN_WIDTH);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN);
    layout->setSpacing(DIALOG_SPACING);

    QFormLayout* form = new QFormLayout;
    form->setSpacing(8);

    // Name
    QWidget* nameWidget = new QWidget(this);
    QHBoxLayout* nameRow = new QHBoxLayout(nameWidget);
    nameRow->setContentsMargins(0, 0, 0, 0);
    nameRow->setSpacing(8);
    m_nameEdit = new QLineEdit(nameWidget);
    m_nameEdit->setMaxLength(NAME_MAX_LEN);
    m_nameEdit->setPlaceholderText(tr("e.g. Cliffe or \"Bot A\""));
    nameRow->addWidget(m_nameEdit, 1);
    m_nameCounter = new QLabel(QStringLiteral("0 / 31"), nameWidget);
    m_nameCounter->setStyleSheet(QStringLiteral("color: #888;"));
    m_nameCounter->setMinimumWidth(48);
    nameRow->addWidget(m_nameCounter);
    form->addRow(tr("Name:"), nameWidget);

    // Skill template
    m_skillCombo = new QComboBox(this);
    for (const QString& s : ServerFiles::knownSkillTemplates())
        m_skillCombo->addItem(s);
    m_skillCombo->setCurrentIndex(2); // Normal as default
    form->addRow(tr("Skill:"), m_skillCombo);

    // Weapon template
    m_weaponCombo = new QComboBox(this);
    m_weaponCombo->addItem(tr("None (use skill template default)"));
    for (const QString& w : ServerFiles::knownWeaponTemplates())
        m_weaponCombo->addItem(w);
    form->addRow(tr("Weapon preference:"), m_weaponCombo);

    // Skin
    m_skinCombo = new QComboBox(this);
    m_skinCombo->addItem(tr("0 — Any skin"));
    m_skinCombo->addItem(tr("1 — Phoenix Connexion / SEAL Team 6"));
    m_skinCombo->addItem(tr("2 — L337 Krew / GSG-9"));
    m_skinCombo->addItem(tr("3 — Arctic Avengers / SAS"));
    m_skinCombo->addItem(tr("4 — Guerrilla Warfare / GIGN"));
    form->addRow(tr("Skin:"), m_skinCombo);

    // Voice pitch
    m_pitchSpinner = new NumberSpinner(this);
    m_pitchSpinner->setRange(PITCH_MIN, PITCH_MAX);
    m_pitchSpinner->setValue(PITCH_DEFAULT);
    m_pitchSpinner->setFixedWidth(PITCH_WIDTH);
    form->addRow(tr("Voice pitch:"), m_pitchSpinner);

    layout->addLayout(form);

    // Buttons
    QDialogButtonBox* box = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okBtn = box->button(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(box);

    // Live updates
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]()
    {
        updateNameCounter();
        updateOkEnabled();
    });

    updateNameCounter();
    updateOkEnabled();
}

void BotEditDialog::updateNameCounter() const
{
    const int len = m_nameEdit->text().length();
    m_nameCounter->setText(QStringLiteral("%1 / %2").arg(len).arg(NAME_MAX_LEN));
    m_nameCounter->setStyleSheet(len > NAME_MAX_LEN
        ? QStringLiteral("color: red;")
        : QStringLiteral("color: #888;"));
}

void BotEditDialog::updateOkEnabled() const
{
    const QString name = m_nameEdit->text().trimmed();
    m_okBtn->setEnabled(name.isEmpty() == false && name.length() <= NAME_MAX_LEN);
}

ServerFiles::BotProfile BotEditDialog::profile() const
{
    ServerFiles::BotProfile p;
    p.name          = m_nameEdit->text().trimmed();
    p.skillTemplate = m_skillCombo->currentText();

    const int wi = m_weaponCombo->currentIndex();
    p.weaponTemplate = (wi == 0)
        ? QString()
        : ServerFiles::knownWeaponTemplates().at(wi - 1);

    p.skin       = m_skinCombo->currentIndex();
    p.voicePitch = m_pitchSpinner->value();
    return p;
}
