#pragma once

#include <QFile>
#include <QString>

// Returns true if the given directory path contains the hlds_run binary.
inline bool isValidServerPath(const QString& dirPath)
{
    if (dirPath.isEmpty()) return false;
    return QFile::exists(dirPath + QStringLiteral("/hlds_run"));
}
