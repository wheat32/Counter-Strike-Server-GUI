#include "appSettingsPage.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSvgRenderer>
#include <QVBoxLayout>

#include "appConfig.h"
#include "dialogs/aboutDialog.h"
#include "serverUtils.h"
#include "themeManager.h"

namespace
{
constexpr int ABOUT_ICON_SIZE = 14;

QIcon renderSvgIcon(const QString& resource, const QColor& color, const int size)
{
    QFile file(resource);
    if (file.open(QIODevice::ReadOnly) == false)
        return {};
    QByteArray data = file.readAll();
    data.replace("currentColor", color.name().toLatin1());
    QSvgRenderer renderer(data);
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    renderer.render(&p);
    return QIcon(px);
}
} // namespace

namespace
{
constexpr int PAGE_MARGIN    = 20;
constexpr int GROUP_SPACING  = 16;
constexpr int RADIO_SPACING  = 8;
constexpr int PATH_SPACING   = 6;
constexpr int LABEL_SPACING  = 4;
constexpr int BROWSE_MAX_W   = 90;
} // namespace

AppSettingsPage::AppSettingsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel(tr("App Settings"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    outerLayout->addWidget(titleLabel);

    QWidget* content = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN);
    contentLayout->setSpacing(GROUP_SPACING);

    // ── Server Locations ──────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Server Locations"), content);
        QVBoxLayout* groupLayout = new QVBoxLayout(group);
        groupLayout->setSpacing(PATH_SPACING);

        // CS 1.6
        QLabel* cs16Label = new QLabel(tr("Counter-Strike 1.6:"), group);
        groupLayout->addWidget(cs16Label);

        QHBoxLayout* cs16Row = new QHBoxLayout();
        cs16Row->setSpacing(PATH_SPACING);
        m_cs16PathEdit = new QLineEdit(group);
        m_cs16PathEdit->setPlaceholderText(tr("Path to directory containing hlds_run"));
        m_cs16PathEdit->setText(AppConfig::instance().cs16ServerPath());
        QPushButton* cs16BrowseBtn = new QPushButton(tr("Browse…"), group);
        cs16BrowseBtn->setMaximumWidth(BROWSE_MAX_W);
        cs16Row->addWidget(m_cs16PathEdit, 1);
        cs16Row->addWidget(cs16BrowseBtn);
        groupLayout->addLayout(cs16Row);

        // Spacer between the two entries
        groupLayout->addSpacing(LABEL_SPACING);

        // CS:CZ
        QLabel* czLabel = new QLabel(tr("Counter-Strike: Condition Zero:"), group);
        groupLayout->addWidget(czLabel);

        QHBoxLayout* czRow = new QHBoxLayout();
        czRow->setSpacing(PATH_SPACING);
        m_czPathEdit = new QLineEdit(group);
        m_czPathEdit->setPlaceholderText(tr("Path to directory containing hlds_run"));
        m_czPathEdit->setText(AppConfig::instance().czServerPath());
        QPushButton* czBrowseBtn = new QPushButton(tr("Browse…"), group);
        czBrowseBtn->setMaximumWidth(BROWSE_MAX_W);
        czRow->addWidget(m_czPathEdit, 1);
        czRow->addWidget(czBrowseBtn);
        groupLayout->addLayout(czRow);

        contentLayout->addWidget(group);

        connect(cs16BrowseBtn, &QPushButton::clicked, this, [this]()
        {
            onBrowse(m_cs16PathEdit, false);
        });
        connect(czBrowseBtn, &QPushButton::clicked, this, [this]()
        {
            onBrowse(m_czPathEdit, true);
        });
        connect(m_cs16PathEdit, &QLineEdit::editingFinished, this, [this]()
        {
            AppConfig::instance().setCs16ServerPath(m_cs16PathEdit->text().trimmed());
        });
        connect(m_czPathEdit, &QLineEdit::editingFinished, this, [this]()
        {
            AppConfig::instance().setCzServerPath(m_czPathEdit->text().trimmed());
        });
    }

    // ── Theme ─────────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Theme"), content);
        QVBoxLayout* themeLayout = new QVBoxLayout(group);
        themeLayout->setSpacing(RADIO_SPACING);

        QRadioButton* systemRadio = new QRadioButton(tr("System"), group);
        QRadioButton* darkRadio   = new QRadioButton(tr("Dark"),   group);
        QRadioButton* lightRadio  = new QRadioButton(tr("Light"),  group);

        themeLayout->addWidget(systemRadio);
        themeLayout->addWidget(darkRadio);
        themeLayout->addWidget(lightRadio);
        contentLayout->addWidget(group);

        switch (AppConfig::instance().theme())
        {
            case AppConfig::Theme::Dark:
                darkRadio->setChecked(true);
                break;
            case AppConfig::Theme::Light:
                lightRadio->setChecked(true);
                break;
            default:
                systemRadio->setChecked(true);
                break;
        }

        connect(systemRadio, &QRadioButton::clicked, this, []()
        {
            AppConfig::instance().setTheme(AppConfig::Theme::System);
            ThemeManager::apply(AppConfig::Theme::System);
        });
        connect(darkRadio, &QRadioButton::clicked, this, []()
        {
            AppConfig::instance().setTheme(AppConfig::Theme::Dark);
            ThemeManager::apply(AppConfig::Theme::Dark);
        });
        connect(lightRadio, &QRadioButton::clicked, this, []()
        {
            AppConfig::instance().setTheme(AppConfig::Theme::Light);
            ThemeManager::apply(AppConfig::Theme::Light);
        });
    }

    // ── Updates ───────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("Updates"), content);
        QVBoxLayout* groupLayout = new QVBoxLayout(group);

        QCheckBox* checkUpdatesBox = new QCheckBox(tr("Check for updates on startup"), group);
        checkUpdatesBox->setChecked(AppConfig::instance().checkForUpdates());
        groupLayout->addWidget(checkUpdatesBox);

        contentLayout->addWidget(group);

        connect(checkUpdatesBox, &QCheckBox::toggled, this, [](const bool checked)
        {
            AppConfig::instance().setCheckForUpdates(checked);
        });
    }

    // ── About ─────────────────────────────────────────────────────────────────
    {
        QGroupBox* group = new QGroupBox(tr("About"), content);
        QHBoxLayout* groupLayout = new QHBoxLayout(group);

        QPushButton* aboutBtn = new QPushButton(tr("About CS Server Manager"), group);
        aboutBtn->setIcon(renderSvgIcon(QStringLiteral(":/assets/box-arrow-up-right.svg"),
                                        palette().color(QPalette::WindowText),
                                        ABOUT_ICON_SIZE));
        aboutBtn->setIconSize(QSize(ABOUT_ICON_SIZE, ABOUT_ICON_SIZE));
        aboutBtn->setLayoutDirection(Qt::RightToLeft);

        groupLayout->addWidget(aboutBtn);
        groupLayout->addStretch();
        contentLayout->addWidget(group);

        connect(aboutBtn, &QPushButton::clicked, this, [this]()
        {
            AboutDialog dlg(this);
            dlg.exec();
        });
    }

    contentLayout->addStretch();
    outerLayout->addWidget(content, 1);
}

void AppSettingsPage::onBrowse(QLineEdit* pathEdit, const bool isCZ)
{
    const QString startDir = pathEdit->text().isEmpty()
        ? QDir::homePath()
        : pathEdit->text();

    const QString chosen = QFileDialog::getExistingDirectory(
        this, tr("Select HLDS Installation Directory"), startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (chosen.isEmpty() == false)
    {
        pathEdit->setText(chosen);
        if (isCZ)
        {
            AppConfig::instance().setCzServerPath(chosen);
        }
        else
        {
            AppConfig::instance().setCs16ServerPath(chosen);
        }
    }
}
