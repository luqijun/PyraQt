#pragma once

#include <QWidget>

class QLabel;
class QTextEdit;

#if PYRAQT_HAS_QSCINTILLA
class QsciLexerPython;
class QsciScintilla;
#endif

namespace pyraqt::ui {

class ScriptEditorWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ScriptEditorWidget(QWidget *parent = nullptr);

    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] QString currentFilePath() const;
    [[nodiscard]] QString currentText() const;
    [[nodiscard]] QString selectedText() const;
    [[nodiscard]] bool isModified() const;
    [[nodiscard]] int currentLine() const;
    [[nodiscard]] int currentColumn() const;
    [[nodiscard]] QString appliedTheme() const;

    void newDocument();
    bool loadFromFile(const QString &filePath);
    bool save();
    bool saveAs(const QString &filePath);
    void setEditorMessage(const QString &message);
    void applyTheme(const QString &themeName);

signals:
    void modificationChanged(bool modified);
    void filePathChanged(const QString &filePath);
    void cursorPositionChanged(int line, int column);

private:
    void setModified(bool modified);

#if PYRAQT_HAS_QSCINTILLA
    QsciScintilla *m_editor = nullptr;
    QsciLexerPython *m_lexer = nullptr;
#else
    QTextEdit *m_editor = nullptr;
    QLabel *m_placeholder = nullptr;
#endif
    QString m_currentFilePath;
    QString m_appliedTheme = QStringLiteral("light");
    bool m_modified = false;
};

} // namespace pyraqt::ui
