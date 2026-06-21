#pragma once

#include <QString>
#include <QStringList>

namespace pyraqt::core {

class PythonRuntimeManager;

class PythonCompletionProvider final {
public:
    [[nodiscard]] static QString prefixBeforeCursor(const QString &text, int cursorPosition);
    [[nodiscard]] static QString objectExpressionForMemberPrefix(const QString &prefix);
    [[nodiscard]] static QString objectExpressionForDottedPrefix(const QString &prefix);
    [[nodiscard]] static QString memberNamePrefixForDottedPrefix(const QString &prefix);
    [[nodiscard]] static QStringList prefixedMemberCompletions(const QString &objectExpression, const QStringList &members);
    [[nodiscard]] static QStringList directMembersForObject(const QStringList &completions, const QString &objectExpression);

    [[nodiscard]] QStringList staticCompletions() const;
    [[nodiscard]] QStringList commonMemberNames() const;
    [[nodiscard]] QStringList runtimeMemberCompletions(PythonRuntimeManager &runtimeManager, const QString &objectExpression) const;
    [[nodiscard]] QStringList runtimeCompletions(PythonRuntimeManager &runtimeManager) const;
    [[nodiscard]] QStringList allCompletions(PythonRuntimeManager *runtimeManager = nullptr) const;
};

} // namespace pyraqt::core
