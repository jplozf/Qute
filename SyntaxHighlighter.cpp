#include "SyntaxHighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupRules();
}

void SyntaxHighlighter::setupRules() {
    highlightingRules.clear();
    HighlightingRule rule;

    QStringList keywordPatterns = {
        "\\bchar\\b", "\\bclass\\b", "\\bconst\\b", "\\bdouble\\b",
        "\\benum\\b", "\\bexplicit\\b", "\\bfriend\\b", "\\binline\\b",
        "\\bint\\b", "\\blong\\b", "\\bnamespace\\b", "\\boperator\\b",
        "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b", "\\bshort\\b",
        "\\bsignals\\b", "\\bsigned\\b", "\\bslots\\b", "\\bstatic\\b",
        "\\bstruct\\b", "\\btemplate\\b", "\\btypedef\\b", "\\btypename\\b",
        "\\bunion\\b", "\\bunsigned\\b", "\\bvirtual\\b", "\\bvoid\\b",
        "\\bvolatile\\b", "\\bbool\\b", "\\bif\\b", "\\belse\\b", "\\bfor\\b",
        "\\bwhile\\b", "\\breturn\\b", "\\binclude\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\".*?\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
}

void SyntaxHighlighter::setTheme(EditorTheme theme) {
    keywordFormat.setFontWeight(QFont::Bold);

    switch (theme) {
    case EditorTheme::Light:
        keywordFormat.setForeground(Qt::darkBlue);
        singleLineCommentFormat.setForeground(Qt::darkGreen);
        quotationFormat.setForeground(Qt::darkRed);
        functionFormat.setForeground(Qt::blue);
        break;
    case EditorTheme::DarkVS:
        keywordFormat.setForeground(QColor(86, 156, 214));    // Light Blue
        singleLineCommentFormat.setForeground(QColor(106, 153, 85)); // Soft Green
        quotationFormat.setForeground(QColor(214, 157, 133));  // Salmon
        functionFormat.setForeground(QColor(220, 220, 170));   // Light Yellow
        break;
    case EditorTheme::Monokai:
        keywordFormat.setForeground(QColor(249, 38, 114));     // Pink/Magenta
        singleLineCommentFormat.setForeground(QColor(117, 113, 94)); // Gray
        quotationFormat.setForeground(QColor(230, 219, 116));  // Yellow
        functionFormat.setForeground(QColor(166, 226, 46));    // Green
        break;
    }
    setupRules();
    rehighlight();
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}