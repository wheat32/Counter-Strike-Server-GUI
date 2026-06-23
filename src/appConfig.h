#pragma once

// AppConfig – persists app-level preferences to ~/.config/CSServerManager/app.json
class AppConfig
{
public:
    enum class Theme { System, Dark, Light };

    static AppConfig& instance();

    Theme theme() const;
    void setTheme(Theme value);

    void resetToDefaults();

private:
    AppConfig();
    void load();
    bool save() const;

    Theme m_theme = Theme::System;
};
