#include "aboutDialog.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "../cli/platformUtils.h"

namespace
{
constexpr int ABOUT_MIN_WIDTH         = 500;
constexpr int ABOUT_MIN_HEIGHT        = 420;
constexpr int ABOUT_LAYOUT_SPACING    = 10;
constexpr int VERSION_GRID_TOP_MARGIN = 4;
constexpr int VERSION_GRID_BOT_MARGIN = 8;
constexpr int VERSION_GRID_H_SPACING  = 8;
constexpr int VERSION_GRID_V_SPACING  = 4;
constexpr int VERSION_COL_STRETCH     = 1;
} // namespace

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    QString appVersion = QStringLiteral("unknown");
    QFile vf(QStringLiteral(":/version.json"));
    if (vf.open(QIODevice::ReadOnly))
    {
        const QJsonObject obj = QJsonDocument::fromJson(vf.readAll()).object();
        vf.close();
        if (obj.contains(QStringLiteral("app_version")))
        {
            appVersion = obj[QStringLiteral("app_version")].toString();
        }
    }

    setWindowTitle(tr("About CS Server Manager"));
    setMinimumSize(ABOUT_MIN_WIDTH, ABOUT_MIN_HEIGHT);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(ABOUT_LAYOUT_SPACING);

    // Title
    QLabel* titleLabel = new QLabel(
        QStringLiteral("<h2 style=\"margin-bottom:2px;\">%1</h2>")
            .arg(tr("CS Server Manager").toHtmlEscaped()),
        this);
    titleLabel->setTextFormat(Qt::RichText);
    layout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel(
        QStringLiteral("<span style=\"color:#888;\">%1</span>")
            .arg(tr("A Qt-based GUI for managing Counter-Strike dedicated servers.")
                     .toHtmlEscaped()),
        this);
    subtitleLabel->setTextFormat(Qt::RichText);
    layout->addWidget(subtitleLabel);

    // Version table
    QWidget*     versionWidget = new QWidget(this);
    QGridLayout* versionGrid   = new QGridLayout(versionWidget);
    versionGrid->setContentsMargins(0, VERSION_GRID_TOP_MARGIN, 0, VERSION_GRID_BOT_MARGIN);
    versionGrid->setHorizontalSpacing(VERSION_GRID_H_SPACING);
    versionGrid->setVerticalSpacing(VERSION_GRID_V_SPACING);
    versionGrid->setColumnStretch(VERSION_COL_STRETCH, 1);

    auto makeKey = [&](const QString& text) -> QLabel*
    {
        QLabel* l = new QLabel(
            QStringLiteral("<b>%1</b>").arg(text.toHtmlEscaped()), versionWidget);
        l->setTextFormat(Qt::RichText);
        return l;
    };

    versionGrid->addWidget(makeKey(tr("App version:")), 0, 0);
    versionGrid->addWidget(new QLabel(appVersion, versionWidget), 0, 1);

    versionGrid->addWidget(makeKey(tr("Qt version:")), 1, 0);
    versionGrid->addWidget(new QLabel(QStringLiteral(QT_VERSION_STR), versionWidget), 1, 1);

    versionGrid->addWidget(makeKey(tr("Package:")), 2, 0);
    versionGrid->addWidget(new QLabel(packageTypeName(), versionWidget), 2, 1);

    layout->addWidget(versionWidget);

    // Credits and acknowledgements
    QTextBrowser* browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setHtml(
        QStringLiteral(
            "<h3>%1</h3>"
            "<ul><li>Nicholas Page (<a href='https://github.com/wheat32'>wheat32</a>)</li></ul>"
            "<h3>%2</h3>"
            "<ul>"
            "<li>%3</li>"
            "<li>%4</li>"
            "</ul>")
        .arg(
            tr("Author"),
            tr("Credits &amp; Acknowledgements"),
            tr("Built with <a href='https://www.qt.io/'>Qt 6</a>"),
            tr("Icons from <a href='https://icons.getbootstrap.com/'>Bootstrap Icons</a>"
               " (MIT License)")));
    layout->addWidget(browser, 1);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(btns);
}
