#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    setTabWidth(4);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::setTheme(EditorTheme theme) {
    m_theme = theme;
    QPalette p = palette();

    switch (theme) {
    case EditorTheme::Light:
        p.setColor(QPalette::Base, Qt::white);
        p.setColor(QPalette::Text, Qt::black);
        break;
    case EditorTheme::DarkVS:
        p.setColor(QPalette::Base, QColor(30, 30, 30));
        p.setColor(QPalette::Text, QColor(220, 220, 220));
        break;
    case EditorTheme::Monokai:
        p.setColor(QPalette::Base, QColor(39, 40, 34));
        p.setColor(QPalette::Text, QColor(248, 248, 242));
        break;
    case EditorTheme::SolarizedDark:
        p.setColor(QPalette::Base, QColor(0, 43, 54));       // Deep Blue-Green
        p.setColor(QPalette::Text, QColor(131, 148, 150));   // Light Cyan-Gray
        break;
    case EditorTheme::Nord:
        p.setColor(QPalette::Base, QColor(46, 52, 64));      // Dark Navy Gray
        p.setColor(QPalette::Text, QColor(216, 222, 233));   // Ice White
        break;
    case EditorTheme::Dracula:
        p.setColor(QPalette::Base, QColor(40, 42, 54));       // Dark Purple-Gray
        p.setColor(QPalette::Text, QColor(248, 248, 242));    // Off-White
        break;
    case EditorTheme::OneDark:
        p.setColor(QPalette::Base, QColor(40, 44, 52));       // Deep Dark Slate
        p.setColor(QPalette::Text, QColor(171, 178, 191));    // Light Gray
        break;
    case EditorTheme::GitHubDark:
        p.setColor(QPalette::Base, QColor(13, 17, 23));       // GitHub Dark Canvas
        p.setColor(QPalette::Text, QColor(240, 246, 252));    // Bright White-Gray
        break;
    case EditorTheme::Coffee:
        p.setColor(QPalette::Base, QColor(245, 239, 230));    // Warm Cream / Beige
        p.setColor(QPalette::Text, QColor(74, 59, 50));       // Espresso Brown
        break;
    }
    setPalette(p);
    highlightCurrentLine();
    lineNumberArea->update();
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor;
        if (m_theme == EditorTheme::Light)
            lineColor = QColor(230, 240, 250);
        else if (m_theme == EditorTheme::DarkVS)
            lineColor = QColor(45, 45, 48);
        else if (m_theme == EditorTheme::Monokai)
            lineColor = QColor(62, 61, 50);
        else if (m_theme == EditorTheme::SolarizedDark)
            lineColor = QColor(7, 54, 66);
        else if (m_theme == EditorTheme::Nord)
            lineColor = QColor(59, 66, 82);
        else if (m_theme == EditorTheme::Dracula)
            lineColor = QColor(68, 71, 90);       // Current Line Highlight
        else if (m_theme == EditorTheme::OneDark)
            lineColor = QColor(44, 49, 58);       // Highlight Line
        else if (m_theme == EditorTheme::GitHubDark)
            lineColor = QColor(22, 27, 34);       // Accent Background Line
        else if (m_theme == EditorTheme::Coffee)
            lineColor = QColor(232, 222, 210);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    QColor sidebarBg;
    QColor textColor;

    if (m_theme == EditorTheme::Light) {
        sidebarBg = QColor(245, 245, 245);
        textColor = QColor(130, 130, 130);
    } else if (m_theme == EditorTheme::DarkVS) {
        sidebarBg = QColor(37, 37, 38);
        textColor = QColor(133, 133, 133);
    } else if (m_theme == EditorTheme::Monokai) {
        sidebarBg = QColor(30, 31, 24);
        textColor = QColor(145, 143, 125);
    } else if (m_theme == EditorTheme::SolarizedDark) {
        sidebarBg = QColor(7, 54, 66);
        textColor = QColor(88, 110, 117);
    } else if (m_theme == EditorTheme::Nord) {
        sidebarBg = QColor(59, 66, 82);
        textColor = QColor(76, 86, 106);
    } else if (m_theme == EditorTheme::Dracula) {
        sidebarBg = QColor(40, 42, 54);
        textColor = QColor(98, 114, 164);
    } else if (m_theme == EditorTheme::OneDark) {
        sidebarBg = QColor(40, 44, 52);
        textColor = QColor(75, 83, 98);
    } else if (m_theme == EditorTheme::GitHubDark) {
        sidebarBg = QColor(13, 17, 23);
        textColor = QColor(110, 118, 129);
    } else { // Coffee
        sidebarBg = QColor(235, 227, 218);
        textColor = QColor(138, 122, 112);
    }

    painter.fillRect(event->rect(), sidebarBg);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingGeometry(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(textColor);
            painter.drawText(0, top, lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingGeometry(block).height());
        ++blockNumber;
    }
}

void CodeEditor::setTabWidth(int width) {
    m_tabWidth = width;
    QFontMetrics fm(font());
    setTabStopDistance(fm.horizontalAdvance(' ') * m_tabWidth);
}

void CodeEditor::setInsertSpacesForTabs(bool enabled) {
    m_insertSpaces = enabled;
}

void CodeEditor::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Tab && m_insertSpaces && !(event->modifiers() & Qt::ShiftModifier)) {
        // Insert soft tabs (spaces)
        QString spaces(m_tabWidth, ' ');
        insertPlainText(spaces);
        event->accept();
        return;
    }

    // Auto-Indentation on Enter / Return
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        QString currentLine = cursor.block().text();

        // Extract leading whitespace from the current line
        QString indentation = "";
        for (QChar c : currentLine) {
            if (c == ' ' || c == '\t') {
                indentation.append(c);
            } else {
                break;
            }
        }

        // Smart indent: Add an extra indent level if line ends with ':' (Python/Go) or '{' (C++/Java/JS)
        QString trimmed = currentLine.trimmed();
        if (trimmed.endsWith(':') || trimmed.endsWith('{')) {
            if (m_insertSpaces) {
                indentation.append(QString(m_tabWidth, ' '));
            } else {
                indentation.append('\t');
            }
        }

        // Let the default enter event execute first (creates the new line)
        QPlainTextEdit::keyPressEvent(event);

        // Insert the carried-over indentation onto the new line
        if (!indentation.isEmpty()) {
            insertPlainText(indentation);
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}