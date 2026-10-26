#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);

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
        else // Monokai
            lineColor = QColor(62, 61, 50);

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
    } else { // Monokai
        sidebarBg = QColor(30, 31, 24);
        textColor = QColor(145, 143, 125);
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