#include "SyntaxHighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupRules();
}

void SyntaxHighlighter::setLanguage(ProgrammingLanguage lang) {
    m_lang = lang;
    setupRules();
    rehighlight();
}

void SyntaxHighlighter::setupRules() {
    highlightingRules.clear();
    HighlightingRule rule;

    QStringList keywordPatterns;

    switch (m_lang) {
    case ProgrammingLanguage::Cpp:
        keywordPatterns = {
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
        break;
    case ProgrammingLanguage::Python:
        keywordPatterns = {
            "\\band\\b", "\\bas\\b", "\\bassert\\b", "\\bbreak\\b", "\\bclass\\b",
            "\\bcontinue\\b", "\\bdef\\b", "\\bdel\\b", "\\belif\\b", "\\belse\\b",
            "\\bexcept\\b", "\\bFalse\\b", "\\bfinally\\b", "\\bfor\\b", "\\bfrom\\b",
            "\\bglobal\\b", "\\bif\\b", "\\bimport\\b", "\\bin\\b", "\\bis\\b",
            "\\blambda\\b", "\\bNone\\b", "\\bnonlocal\\b", "\\bnot\\b", "\\bor\\b",
            "\\bpass\\b", "\\braise\\b", "\\breturn\\b", "\\bTrue\\b", "\\btry\\b",
            "\\bwhile\\b", "\\bwith\\b", "\\byield\\b"
        };
        break;
    case ProgrammingLanguage::Go:
        keywordPatterns = {
            "\\bbreak\\b", "\\bcase\\b", "\\bchan\\b", "\\bconst\\b", "\\bcontinue\\b",
            "\\bdefault\\b", "\\bdefer\\b", "\\belse\\b", "\\bfatal\\b", "\\bfallthrough\\b",
            "\\bfor\\b", "\\bfunc\\b", "\\bgo\\b", "\\bgoto\\b", "\\bif\\b",
            "\\bimport\\b", "\\binterface\\b", "\\bmap\\b", "\\bpackage\\b", "\\brange\\b",
            "\\breturn\\b", "\\bselect\\b", "\\bstruct\\b", "\\bswitch\\b", "\\btype\\b",
            "\\bvar\\b"
        };
        break;
    case ProgrammingLanguage::JavaScript:
        keywordPatterns = {
            "\\basync\\b", "\\bawait\\b", "\\bbreak\\b", "\\bcase\\b", "\\bcatch\\b",
            "\\bclass\\b", "\\bconst\\b", "\\bcontinue\\b", "\\bdebugger\\b", "\\bdefault\\b",
            "\\bdelete\\b", "\\bdo\\b", "\\belse\\b", "\\benum\\b", "\\bexport\\b",
            "\\bextends\\b", "\\bfalse\\b", "\\bfinally\\b", "\\bfor\\b", "\\bfunction\\b",
            "\\bif\\b", "\\bimport\\b", "\\bin\\b", "\\binstanceof\\b", "\\bnew\\b",
            "\\bnull\\b", "\\breturn\\b", "\\bsuper\\b", "\\bswitch\\b", "\\bthis\\b",
            "\\bthrow\\b", "\\btrue\\b", "\\btry\\b", "\\btypeof\\b", "\\bvar\\b",
            "\\bvoid\\b", "\\bwhile\\b", "\\bwith\\b", "\\byield\\b", "\\blet\\b", "\\bconst\\b"
        };
        break;
    }

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Comments (Python uses '#', C++/Go/JS use '//')
    if (m_lang == ProgrammingLanguage::Python) {
        rule.pattern = QRegularExpression("#[^\n]*");
    } else {
        rule.pattern = QRegularExpression("//[^\n]*");
    }
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Quotations / Strings
    rule.pattern = QRegularExpression("\".*?\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
}

void SyntaxHighlighter::setTheme(EditorTheme theme) {
    m_theme = theme;
    keywordFormat.setFontWeight(QFont::Bold);

    switch (theme) {
    case EditorTheme::Light:
        keywordFormat.setForeground(Qt::darkBlue);
        singleLineCommentFormat.setForeground(Qt::darkGreen);
        quotationFormat.setForeground(Qt::darkRed);
        functionFormat.setForeground(Qt::blue);
        break;
    case EditorTheme::DarkVS:
        keywordFormat.setForeground(QColor(86, 156, 214));
        singleLineCommentFormat.setForeground(QColor(106, 153, 85));
        quotationFormat.setForeground(QColor(214, 157, 133));
        functionFormat.setForeground(QColor(220, 220, 170));
        break;
    case EditorTheme::Monokai:
        keywordFormat.setForeground(QColor(249, 38, 114));
        singleLineCommentFormat.setForeground(QColor(117, 113, 94));
        quotationFormat.setForeground(QColor(230, 219, 116));
        functionFormat.setForeground(QColor(166, 226, 46));
        break;
    case EditorTheme::SolarizedDark:
        keywordFormat.setForeground(QColor(38, 139, 210));
        singleLineCommentFormat.setForeground(QColor(133, 153, 0));
        quotationFormat.setForeground(QColor(42, 161, 152));
        functionFormat.setForeground(QColor(181, 137, 0));
        break;
    case EditorTheme::Nord:
        keywordFormat.setForeground(QColor(129, 161, 193));
        singleLineCommentFormat.setForeground(QColor(97, 110, 136));
        quotationFormat.setForeground(QColor(163, 190, 140));
        functionFormat.setForeground(QColor(136, 192, 208));
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