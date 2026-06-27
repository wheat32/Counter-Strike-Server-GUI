#include "mapsPage.h"

#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "appConfig.h"
#include "serverFiles.h"

namespace
{
constexpr int PAGE_MARGIN   = 20;
constexpr int GROUP_SPACING = 16;
constexpr int CHECK_SPACING = 6;

enum class MapType { Defuse, Hostage, Assassination, Aim, FightYard, Other };

MapType detectType(const QString& name)
{
    if (name.startsWith(QStringLiteral("de_"),  Qt::CaseInsensitive)) return MapType::Defuse;
    if (name.startsWith(QStringLiteral("cs_"),  Qt::CaseInsensitive)) return MapType::Hostage;
    if (name.startsWith(QStringLiteral("as_"),  Qt::CaseInsensitive)) return MapType::Assassination;
    if (name.startsWith(QStringLiteral("aim_"), Qt::CaseInsensitive)) return MapType::Aim;
    if (name.startsWith(QStringLiteral("fy_"),  Qt::CaseInsensitive)) return MapType::FightYard;
    return MapType::Other;
}

bool isCZVariant(const QString& name)
{
    return name.endsWith(QStringLiteral("_cz"), Qt::CaseInsensitive);
}
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

        m_showDefuse        = new QCheckBox(tr("Defuse (de_)"),         group);
        m_showHostage       = new QCheckBox(tr("Hostage rescue (cs_)"), group);
        m_showAssassination = new QCheckBox(tr("Assassination (as_)"),  group);
        m_showAim           = new QCheckBox(tr("Aim training (aim_)"),  group);
        m_showFightYard     = new QCheckBox(tr("Fight Yard (fy_)"),     group);
        m_showOther         = new QCheckBox(tr("Other"),                group);

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
    applyFilters();
}

void MapsPage::applyFilters()
{
    const AppConfig::Game game = AppConfig::instance().selectedGame();
    const QString savedMap = (game == AppConfig::Game::CZ)
        ? AppConfig::instance().czStartMap()
        : AppConfig::instance().cs16StartMap();

    const bool wantStandard = m_showStandard->isChecked();
    const bool wantCZ       = m_showCZ->isChecked();
    const bool wantDefuse   = m_showDefuse->isChecked();
    const bool wantHostage  = m_showHostage->isChecked();
    const bool wantAssassin = m_showAssassination->isChecked();
    const bool wantAim      = m_showAim->isChecked();
    const bool wantFY       = m_showFightYard->isChecked();
    const bool wantOther    = m_showOther->isChecked();

    m_mapCombo->blockSignals(true);
    m_mapCombo->clear();

    for (const QString& map : std::as_const(m_allMaps))
    {
        const bool czVariant = isCZVariant(map);
        if (czVariant  && !wantCZ)       continue;
        if (!czVariant && !wantStandard) continue;

        bool typeOk = false;
        switch (detectType(map))
        {
            case MapType::Defuse:        typeOk = wantDefuse;   break;
            case MapType::Hostage:       typeOk = wantHostage;  break;
            case MapType::Assassination: typeOk = wantAssassin; break;
            case MapType::Aim:           typeOk = wantAim;      break;
            case MapType::FightYard:     typeOk = wantFY;       break;
            case MapType::Other:         typeOk = wantOther;    break;
        }
        if (!typeOk) continue;

        m_mapCombo->addItem(map);
    }

    // Restore the saved selection; fall back to first entry if it was filtered out.
    const int idx = m_mapCombo->findText(savedMap, Qt::MatchFixedString);
    m_mapCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    m_mapCombo->blockSignals(false);
}
