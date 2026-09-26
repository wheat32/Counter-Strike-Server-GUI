#include "mapsPage.h"

#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "appConfig.h"
#include "dialogs/mapBrowserDialog.h"
#include "serverFiles.h"

using MapTypes::MapType;

namespace
{
constexpr int PAGE_MARGIN   = 20;
constexpr int GROUP_SPACING = 16;
constexpr int CHECK_SPACING = 6;
} // namespace

MapsPage::MapsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel(tr("Maps"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    outerLayout->addWidget(titleLabel);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scroll, 1);

    QWidget* content = new QWidget(scroll);
    scroll->setWidget(content);

    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN);
    contentLayout->setSpacing(GROUP_SPACING);

    // ── Starting Map ──────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Starting Map"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);

        grp->addWidget(new QLabel(tr("Map the server loads on startup:"), group));

        m_mapCombo = new QComboBox(group);
        grp->addWidget(m_mapCombo);

        m_browseBtn = new QPushButton(tr("Browse All Maps…"), group);
        grp->addWidget(m_browseBtn, 0, Qt::AlignLeft);
        connect(m_browseBtn, &QPushButton::clicked, this, &MapsPage::openMapBrowser);

        contentLayout->addWidget(group);

        connect(m_mapCombo, &QComboBox::currentTextChanged, this, [this](const QString& map)
        {
            if (map.isEmpty()) return;
            const AppConfig::Game game = AppConfig::instance().selectedGame();
            if (game == AppConfig::Game::CZ)
                AppConfig::instance().setCzStartMap(map);
            else
                AppConfig::instance().setCs16StartMap(map);
            emit mapSelected(map);
            emit settingChanged();
        });
    }

    // ── Filters ───────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Filters"), content);
        QVBoxLayout* grp = new QVBoxLayout(group);
        grp->setSpacing(CHECK_SPACING);

        // Version header
        QLabel* versionLbl = new QLabel(tr("Map version:"), group);
        QFont bold = versionLbl->font();
        bold.setBold(true);
        versionLbl->setFont(bold);
        grp->addWidget(versionLbl);

        m_showStandard = new QCheckBox(tr("Standard"), group);
        m_showCZ       = new QCheckBox(tr("CZ variant (ends in _cz)"), group);
        m_showStandard->setChecked(true);
        m_showCZ->setChecked(true);
        grp->addWidget(m_showStandard);
        grp->addWidget(m_showCZ);

        QFrame* divider = new QFrame(group);
        divider->setFrameShape(QFrame::HLine);
        divider->setObjectName(QStringLiteral("sidebarDivider"));
        grp->addWidget(divider);

        // Type header
        QLabel* typeLbl = new QLabel(tr("Map type:"), group);
        typeLbl->setFont(bold);
        grp->addWidget(typeLbl);

        m_showDefuse        = new QCheckBox(MapTypes::typeLabel(MapType::Defuse),        group);
        m_showHostage       = new QCheckBox(MapTypes::typeLabel(MapType::Hostage),       group);
        m_showAssassination = new QCheckBox(MapTypes::typeLabel(MapType::Assassination), group);
        m_showAim           = new QCheckBox(MapTypes::typeLabel(MapType::Aim),           group);
        m_showFightYard     = new QCheckBox(MapTypes::typeLabel(MapType::FightYard),     group);
        m_showOther         = new QCheckBox(MapTypes::typeLabel(MapType::Other),         group);

        m_showDefuse->setChecked(true);
        m_showHostage->setChecked(true);
        m_showAssassination->setChecked(true);
        m_showAim->setChecked(true);
        m_showFightYard->setChecked(true);
        m_showOther->setChecked(true);

        grp->addWidget(m_showDefuse);
        grp->addWidget(m_showHostage);
        grp->addWidget(m_showAssassination);
        grp->addWidget(m_showAim);
        grp->addWidget(m_showFightYard);
        grp->addWidget(m_showOther);

        contentLayout->addWidget(group);

        const auto connectFilter = [this](QCheckBox* cb)
        {
            connect(cb, &QCheckBox::toggled, this, &MapsPage::applyFilters);
        };
        connectFilter(m_showStandard);
        connectFilter(m_showCZ);
        connectFilter(m_showDefuse);
        connectFilter(m_showHostage);
        connectFilter(m_showAssassination);
        connectFilter(m_showAim);
        connectFilter(m_showFightYard);
        connectFilter(m_showOther);
    }

    contentLayout->addStretch();

    loadForGame(AppConfig::instance().selectedGame());
}

void MapsPage::loadForGame(const AppConfig::Game game)
{
    m_allMaps = ServerFiles::scanMaps(game);
    m_browseBtn->setEnabled(m_allMaps.isEmpty() == false);
    applyFilters();
}

QCheckBox* MapsPage::typeFilter(const MapType type) const
{
    switch (type)
    {
        case MapType::Defuse:
            return m_showDefuse;
        case MapType::Hostage:
            return m_showHostage;
        case MapType::Assassination:
            return m_showAssassination;
        case MapType::Aim:
            return m_showAim;
        case MapType::FightYard:
            return m_showFightYard;
        case MapType::Other:
            break;
    }
    return m_showOther;
}

void MapsPage::showInFilters(const QString& map)
{
    // Each newly checked box re-runs applyFilters().
    QCheckBox* versionFilter = MapTypes::isCZVariant(map) ? m_showCZ : m_showStandard;
    versionFilter->setChecked(true);
    typeFilter(MapTypes::detectType(map))->setChecked(true);
}

void MapsPage::openMapBrowser()
{
    MapBrowserDialog dlg(m_allMaps, this);
    if (dlg.exec() != QDialog::Accepted) return;

    const QString map = dlg.selectedMap();
    if (map.isEmpty()) return;

    // The dialog lists every map, so this one may be filtered out.
    showInFilters(map);

    // Selecting it saves the start map and notifies the Server page.
    const int idx = m_mapCombo->findText(map, Qt::MatchFixedString);
    if (idx >= 0)
    {
        m_mapCombo->setCurrentIndex(idx);
    }
}

void MapsPage::setStartMap(const QString& map)
{
    showInFilters(map);

    const int idx = m_mapCombo->findText(map, Qt::MatchFixedString);
    if (idx < 0) return;
    m_mapCombo->blockSignals(true);
    m_mapCombo->setCurrentIndex(idx);
    m_mapCombo->blockSignals(false);
}

void MapsPage::applyFilters()
{
    const AppConfig::Game game = AppConfig::instance().selectedGame();
    const QString savedMap = (game == AppConfig::Game::CZ)
        ? AppConfig::instance().czStartMap()
        : AppConfig::instance().cs16StartMap();

    const bool wantStandard = m_showStandard->isChecked();
    const bool wantCZ       = m_showCZ->isChecked();

    m_mapCombo->blockSignals(true);
    m_mapCombo->clear();

    for (const QString& map : std::as_const(m_allMaps))
    {
        const bool czVariant = MapTypes::isCZVariant(map);
        if (czVariant && wantCZ == false) continue;
        if (czVariant == false && wantStandard == false) continue;

        if (typeFilter(MapTypes::detectType(map))->isChecked() == false) continue;

        m_mapCombo->addItem(map);
    }

    // Restore the saved selection; fall back to first entry if it was filtered out.
    const int idx = m_mapCombo->findText(savedMap, Qt::MatchFixedString);
    m_mapCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    m_mapCombo->blockSignals(false);
}
