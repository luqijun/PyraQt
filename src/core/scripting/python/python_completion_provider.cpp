#include "core/scripting/python/python_completion_provider.h"

#include "core/scripting/python/python_runtime_manager.h"

#include <QChar>
#include <QRegularExpression>
#include <QSet>

namespace pyraqt::core {
namespace {

QStringList uniqueSorted(QStringList words)
{
    QSet<QString> seen;
    QStringList result;
    result.reserve(words.size());
    for (const QString &word : words) {
        if (word.trimmed().isEmpty() || seen.contains(word)) {
            continue;
        }
        seen.insert(word);
        result.push_back(word);
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

} // namespace

QString PythonCompletionProvider::prefixBeforeCursor(const QString &text, int cursorPosition)
{
    const int end = qBound(0, cursorPosition, text.size());
    int start = end;
    while (start > 0) {
        const QChar ch = text.at(start - 1);
        if (!(ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('.'))) {
            break;
        }
        --start;
    }
    return text.mid(start, end - start);
}

QString PythonCompletionProvider::objectExpressionForMemberPrefix(const QString &prefix)
{
    if (!prefix.endsWith(QLatin1Char('.'))) {
        return {};
    }

    QString expression = prefix;
    while (expression.endsWith(QLatin1Char('.'))) {
        expression.chop(1);
    }
    static const QRegularExpression validExpression(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*$"));
    return validExpression.match(expression).hasMatch() ? expression : QString();
}

QString PythonCompletionProvider::objectExpressionForDottedPrefix(const QString &prefix)
{
    const int dotIndex = prefix.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0) {
        return {};
    }

    const QString expression = prefix.left(dotIndex);
    static const QRegularExpression validExpression(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*$"));
    return validExpression.match(expression).hasMatch() ? expression : QString();
}

QString PythonCompletionProvider::memberNamePrefixForDottedPrefix(const QString &prefix)
{
    const int dotIndex = prefix.lastIndexOf(QLatin1Char('.'));
    if (dotIndex < 0 || dotIndex == prefix.size() - 1) {
        return {};
    }

    const QString memberPrefix = prefix.mid(dotIndex + 1);
    static const QRegularExpression validMemberPrefix(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    return validMemberPrefix.match(memberPrefix).hasMatch() ? memberPrefix : QString();
}

QStringList PythonCompletionProvider::prefixedMemberCompletions(const QString &objectExpression, const QStringList &members)
{
    if (objectExpression.isEmpty()) {
        return {};
    }

    QStringList words;
    words.reserve(members.size());
    for (const QString &member : members) {
        if (member.trimmed().isEmpty()) {
            continue;
        }
        words.push_back(QStringLiteral("%1.%2").arg(objectExpression, member.trimmed()));
    }
    return uniqueSorted(words);
}

QStringList PythonCompletionProvider::directMembersForObject(const QStringList &completions, const QString &objectExpression)
{
    if (objectExpression.isEmpty()) {
        return {};
    }

    const QString prefix = objectExpression + QLatin1Char('.');
    QStringList members;
    for (const QString &completion : completions) {
        if (!completion.startsWith(prefix)) {
            continue;
        }
        const QString remainder = completion.mid(prefix.size());
        const QString member = remainder.section(QLatin1Char('.'), 0, 0);
        if (!member.isEmpty()) {
            members.push_back(member);
        }
    }
    return uniqueSorted(members);
}

QStringList PythonCompletionProvider::staticCompletions() const
{
    QStringList words{
        QStringLiteral("False"),
        QStringLiteral("None"),
        QStringLiteral("True"),
        QStringLiteral("and"),
        QStringLiteral("as"),
        QStringLiteral("assert"),
        QStringLiteral("async"),
        QStringLiteral("await"),
        QStringLiteral("break"),
        QStringLiteral("class"),
        QStringLiteral("continue"),
        QStringLiteral("def"),
        QStringLiteral("del"),
        QStringLiteral("elif"),
        QStringLiteral("else"),
        QStringLiteral("except"),
        QStringLiteral("finally"),
        QStringLiteral("for"),
        QStringLiteral("from"),
        QStringLiteral("global"),
        QStringLiteral("if"),
        QStringLiteral("import"),
        QStringLiteral("in"),
        QStringLiteral("is"),
        QStringLiteral("lambda"),
        QStringLiteral("nonlocal"),
        QStringLiteral("not"),
        QStringLiteral("or"),
        QStringLiteral("pass"),
        QStringLiteral("raise"),
        QStringLiteral("return"),
        QStringLiteral("try"),
        QStringLiteral("while"),
        QStringLiteral("with"),
        QStringLiteral("yield"),
        QStringLiteral("abs"),
        QStringLiteral("all"),
        QStringLiteral("any"),
        QStringLiteral("bool"),
        QStringLiteral("dict"),
        QStringLiteral("dir"),
        QStringLiteral("enumerate"),
        QStringLiteral("float"),
        QStringLiteral("getattr"),
        QStringLiteral("hasattr"),
        QStringLiteral("help"),
        QStringLiteral("int"),
        QStringLiteral("len"),
        QStringLiteral("list"),
        QStringLiteral("map"),
        QStringLiteral("max"),
        QStringLiteral("min"),
        QStringLiteral("open"),
        QStringLiteral("print"),
        QStringLiteral("range"),
        QStringLiteral("repr"),
        QStringLiteral("set"),
        QStringLiteral("setattr"),
        QStringLiteral("sorted"),
        QStringLiteral("str"),
        QStringLiteral("sum"),
        QStringLiteral("tuple"),
        QStringLiteral("type"),
        QStringLiteral("zip"),
        QStringLiteral("import os"),
        QStringLiteral("import pathlib"),
        QStringLiteral("import sys"),
        QStringLiteral("import pyra"),
        QStringLiteral("from pyra import app"),
        QStringLiteral("pyra"),
        QStringLiteral("pyra.app"),
        QStringLiteral("pyra.app.name"),
        QStringLiteral("pyra.app.version"),
        QStringLiteral("pyra.app.interpreter_path"),
        QStringLiteral("pyra.ui"),
        QStringLiteral("pyra.ui.show_message"),
        QStringLiteral("pyra.ui.set_status"),
        QStringLiteral("pyra.fs"),
        QStringLiteral("pyra.fs.read_text"),
        QStringLiteral("pyra.fs.write_text"),
        QStringLiteral("pyra.log"),
        QStringLiteral("pyra.log.info"),
        QStringLiteral("pyra.log.warn"),
        QStringLiteral("pyra.log.error"),
        QStringLiteral("pyra.commands"),
        QStringLiteral("pyra.commands.register"),
        QStringLiteral("pyra.commands.run"),
        QStringLiteral("pyra.macros"),
        QStringLiteral("pyra.macros.load"),
        QStringLiteral("pyra.macros.trigger"),
        QStringLiteral("pyra.expressions"),
        QStringLiteral("pyra.expressions.register"),
        QStringLiteral("pyra.expressions.evaluate"),
        QStringLiteral("pyra.processing"),
        QStringLiteral("pyra.processing.register"),
        QStringLiteral("pyra.processing.run"),
        QStringLiteral("iface"),
        QStringLiteral("iface.app"),
        QStringLiteral("iface.ui"),
        QStringLiteral("iface.fs"),
        QStringLiteral("iface.log"),
        QStringLiteral("iface.commands"),
        QStringLiteral("iface.processing"),
        QStringLiteral("iface.macros"),
        QStringLiteral("iface.expressions"),
        QStringLiteral("iface.mainWindow"),
        QStringLiteral("iface.showMessage"),
        QStringLiteral("iface.setStatus"),
        QStringLiteral("iface.registerCommand"),
    };
    return uniqueSorted(words);
}

QStringList PythonCompletionProvider::commonMemberNames() const
{
    return uniqueSorted({
        QStringLiteral("__class__"),
        QStringLiteral("__dict__"),
        QStringLiteral("__doc__"),
        QStringLiteral("__init__"),
        QStringLiteral("__module__"),
        QStringLiteral("__name__"),
        QStringLiteral("__repr__"),
        QStringLiteral("__str__"),
        QStringLiteral("append"),
        QStringLiteral("clear"),
        QStringLiteral("copy"),
        QStringLiteral("count"),
        QStringLiteral("endswith"),
        QStringLiteral("extend"),
        QStringLiteral("format"),
        QStringLiteral("get"),
        QStringLiteral("index"),
        QStringLiteral("insert"),
        QStringLiteral("items"),
        QStringLiteral("join"),
        QStringLiteral("keys"),
        QStringLiteral("lower"),
        QStringLiteral("pop"),
        QStringLiteral("remove"),
        QStringLiteral("replace"),
        QStringLiteral("setdefault"),
        QStringLiteral("sort"),
        QStringLiteral("split"),
        QStringLiteral("startswith"),
        QStringLiteral("strip"),
        QStringLiteral("upper"),
        QStringLiteral("values"),
    });
}

QStringList PythonCompletionProvider::runtimeMemberCompletions(PythonRuntimeManager &runtimeManager, const QString &objectExpression) const
{
    return prefixedMemberCompletions(objectExpression, runtimeManager.completeMembers(objectExpression));
}

QStringList PythonCompletionProvider::runtimeCompletions(PythonRuntimeManager &runtimeManager) const
{
    if (!runtimeManager.initializeEmbedded()) {
        return {};
    }

    const ScriptResult result = runtimeManager.evalString(QStringLiteral("[name for name in globals() if not name.startswith('__')]"));
    if (!result.success || result.resultText.isEmpty()) {
        return {};
    }

    QString text = result.resultText;
    text.remove(QLatin1Char('['));
    text.remove(QLatin1Char(']'));
    text.remove(QLatin1Char('\''));
    QStringList words;
    for (const QString &word : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        words.push_back(word.trimmed());
    }
    return uniqueSorted(words);
}

QStringList PythonCompletionProvider::allCompletions(PythonRuntimeManager *runtimeManager) const
{
    QStringList words = staticCompletions();
    if (runtimeManager != nullptr) {
        words.append(runtimeCompletions(*runtimeManager));
    }
    return uniqueSorted(words);
}

} // namespace pyraqt::core
