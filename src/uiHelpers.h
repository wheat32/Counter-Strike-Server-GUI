#pragma once

// Shared UI helpers used across multiple source files.

// Braille spinner frames — use with kSpinnerFrameCount for animated progress.
static constexpr const char* kSpinnerFrames[] =
{
    "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
};
static constexpr int kSpinnerFrameCount = 10;

// Returns true for the common truthy strings used in config/CLI output.
inline bool isOnString(const QString& v)
{
    return v == QLatin1String("on")
        || v == QLatin1String("true")
        || v == QLatin1String("1")
        || v == QLatin1String("enabled");
}
