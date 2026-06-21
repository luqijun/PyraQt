#pragma once

#include <QString>
#include <QMetaType>

namespace pyraqt::core {

struct ScriptResult {
    bool success = false;
    QString resultText;
    QString stdoutText;
    QString stderrText;
    QString traceback;
    int exitCode = 0;
    qint64 durationMs = 0;
};

} // namespace pyraqt::core

Q_DECLARE_METATYPE(pyraqt::core::ScriptResult)
