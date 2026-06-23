#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#include "themeManager.h"

namespace ThemeManager
{

namespace
{
void applyDark()
{
    QPalette palette;
    constexpr QColor bg(0x0f, 0x11, 0x17);
    constexpr QColor surface(0x1a, 0x1d, 0x27);
    constexpr QColor border(0x2c, 0x33, 0x47);
    constexpr QColor accent(0xc8, 0xa8, 0x00);
    constexpr QColor textPrimary(0xe2, 0xe8, 0xf0);
    constexpr QColor textSecondary(0x88, 0x96, 0xa8);

    palette.setColor(QPalette::Window,          bg);
    palette.setColor(QPalette::WindowText,      textPrimary);
    palette.setColor(QPalette::Base,            surface);
    palette.setColor(QPalette::AlternateBase,   bg);
    palette.setColor(QPalette::ToolTipBase,     surface);
    palette.setColor(QPalette::ToolTipText,     textPrimary);
    palette.setColor(QPalette::Text,            textPrimary);
    palette.setColor(QPalette::BrightText,      Qt::white);
    palette.setColor(QPalette::Button,          surface);
    palette.setColor(QPalette::ButtonText,      textPrimary);
    palette.setColor(QPalette::Link,            accent);
    palette.setColor(QPalette::Highlight,       accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::PlaceholderText, textSecondary);
    palette.setColor(QPalette::Mid,             border);
    palette.setColor(QPalette::Dark,            border);
    palette.setColor(QPalette::Midlight,        surface);
    palette.setColor(QPalette::Shadow,          QColor(0x00, 0x00, 0x00, 0xa0));
    QApplication::setPalette(palette);

    QFile f(QStringLiteral(":/style_dark.qss"));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    }
}

void applyLight()
{
    QPalette palette;
    constexpr QColor bg(0xf0, 0xf4, 0xf8);
    constexpr QColor surface(0xff, 0xff, 0xff);
    constexpr QColor border(0xd0, 0xda, 0xe8);
    constexpr QColor accent(0x8a, 0x70, 0x00);
    constexpr QColor textPrimary(0x1a, 0x1d, 0x27);
    constexpr QColor textSecondary(0x4a, 0x58, 0x70);

    palette.setColor(QPalette::Window,          bg);
    palette.setColor(QPalette::WindowText,      textPrimary);
    palette.setColor(QPalette::Base,            surface);
    palette.setColor(QPalette::AlternateBase,   bg);
    palette.setColor(QPalette::ToolTipBase,     surface);
    palette.setColor(QPalette::ToolTipText,     textPrimary);
    palette.setColor(QPalette::Text,            textPrimary);
    palette.setColor(QPalette::BrightText,      Qt::black);
    palette.setColor(QPalette::Button,          surface);
    palette.setColor(QPalette::ButtonText,      textPrimary);
    palette.setColor(QPalette::Link,            accent);
    palette.setColor(QPalette::Highlight,       accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::PlaceholderText, textSecondary);
    palette.setColor(QPalette::Mid,             border);
    palette.setColor(QPalette::Dark,            border);
    palette.setColor(QPalette::Midlight,        QColor(0xe8, 0xef, 0xf8));
    palette.setColor(QPalette::Shadow,          QColor(0x00, 0x00, 0x00, 0x28));
    QApplication::setPalette(palette);

    QFile f(QStringLiteral(":/style_light.qss"));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    }
}

bool systemIsDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
#endif
}

} // anonymous namespace

void apply(const AppConfig::Theme theme)
{
    bool useDark = false;
    switch (theme)
    {
        case AppConfig::Theme::Dark:
            useDark = true;
            break;
        case AppConfig::Theme::Light:
            useDark = false;
            break;
        default:
            useDark = systemIsDark();
            break;
    }

    if (useDark)
    {
        applyDark();
    }
    else
    {
        applyLight();
    }
}

} // namespace ThemeManager
