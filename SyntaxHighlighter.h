#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

enum class EditorTheme {
    Light,
    DarkVS,
    Monokai,
    SolarizedDark,
    Nord
};

enum class ProgrammingLanguage {
    Cpp,
    Python,
    Go,
    JavaScript,
    Java,
    Html,
    Xml
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);
    void setTheme(EditorTheme theme);

    void setLanguage(ProgrammingLanguage lang);
    ProgrammingLanguage language() const { return m_lang; }

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat tagFormat;

    EditorTheme m_theme = EditorTheme::Light;
    ProgrammingLanguage m_lang = ProgrammingLanguage::Cpp;

    void setupRules();
};