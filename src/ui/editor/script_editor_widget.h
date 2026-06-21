#pragma once

#include <QWidget>

class QEvent;
class QLabel;
class QTextEdit;

#if PYRAQT_HAS_QSCINTILLA
class QsciAPIs;
class QsciLexerPython;
class QsciScintilla;
#endif

namespace pyraqt::core {
class PythonRuntimeManager;
}

namespace pyraqt::ui {

class ScriptEditorWidget final : public QWidget {
    Q_OBJECT

public:
    enum class DocumentMode {
        PlainText,
        Python,
    };

    explicit ScriptEditorWidget(core::PythonRuntimeManager *runtimeManager = nullptr, QWidget *parent = nullptr);
    ~ScriptEditorWidget() override;

    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] QString currentFilePath() const;
    [[nodiscard]] QString currentText() const;
    [[nodiscard]] QString selectedText() const;
    [[nodiscard]] bool isModified() const;
    [[nodiscard]] int currentLine() const;
    [[nodiscard]] int currentColumn() const;
    [[nodiscard]] QString appliedTheme() const;
    [[nodiscard]] DocumentMode documentMode() const;
    [[nodiscard]] bool isPythonDocument() const;
    [[nodiscard]] bool codeCompletionEnabled() const;
    [[nodiscard]] bool dotCompletionEnabled() const;
    [[nodiscard]] QStringList completionWords() const;
    [[nodiscard]] QStringList lastMemberCompletionWordsForTesting() const;
    void setTextForTesting(const QString &text);
    void triggerDotCompletionForTesting();
    void typeMemberTextForTesting(const QString &text);
    void refreshMemberCompletionForTesting();

    void newDocument();
    bool loadFromFile(const QString &filePath);
    bool save();
    bool saveAs(const QString &filePath);
    void setCurrentFilePath(const QString &filePath);
    void setEditorMessage(const QString &message);
    void applyTheme(const QString &themeName);

signals:
    void modificationChanged(bool modified);
    void filePathChanged(const QString &filePath);
    void cursorPositionChanged(int line, int column);

private:
    void retranslateUi();
    void setDocumentMode(DocumentMode mode);
    void updateDocumentModeFromPath(const QString &filePath);
    void configureCodeCompletion();
    [[nodiscard]] QStringList editorMemberCompletions() const;
    [[nodiscard]] QString contextCodeBeforeCursor(int prefixLength) const;
    void insertTextAtCursor(const QString &text);
    void insertDotAndShowMemberCompletion();
    void showDotMemberCompletion();
    void setModified(bool modified);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;

#if PYRAQT_HAS_QSCINTILLA
    QsciScintilla *m_editor = nullptr;
    QsciLexerPython *m_lexer = nullptr;
    QsciAPIs *m_apis = nullptr;
#else
    QTextEdit *m_editor = nullptr;
    QLabel *m_placeholder = nullptr;
#endif
    QString m_currentFilePath;
    core::PythonRuntimeManager *m_runtimeManager = nullptr;
    QString m_appliedTheme = QStringLiteral("light");
    QStringList m_completionWords;
    QStringList m_editorMemberWords;
    QStringList m_lastMemberCompletionWords;
    DocumentMode m_documentMode = DocumentMode::PlainText;
    bool m_codeCompletionEnabled = false;
    bool m_dotCompletionEnabled = false;
    bool m_modified = false;
    bool m_hasCustomPlaceholderMessage = false;
};

} // namespace pyraqt::ui
