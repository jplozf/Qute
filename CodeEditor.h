#pragma once

#include <QPlainTextEdit>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QFileInfo>
#include "SyntaxHighlighter.h"

class LineNumberArea;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setTheme(EditorTheme theme);
    EditorTheme currentTheme() const { return m_theme; }
    void setTabWidth(int width);
    void setInsertSpacesForTabs(bool enabled);
    int tabWidth() const { return m_tabWidth; }
    bool insertSpacesForTabs() const { return m_insertSpaces; }
    bool minimizeToTray() const { return m_minimizeToTray; }
    void setLanguage(ProgrammingLanguage lang) { m_language = lang; }
    ProgrammingLanguage language() const { return m_language; }
    void setSearchHighlight(const QString &searchString, QTextDocument::FindFlags flags);

    void startWatchingFile(const QString &filePath);
    void stopWatchingFile();
    bool isSaving() const { return m_isSaving; }
    void setSaving(bool saving) { m_isSaving = saving; }

signals:
    void externalFileChanged(const QString &filePath);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void handleFileChanged(const QString &path);

private:
    QWidget *lineNumberArea;
    EditorTheme m_theme = EditorTheme::Light;
    int m_tabWidth = 4;
    bool m_insertSpaces = true;
    bool m_minimizeToTray = true;
    ProgrammingLanguage m_language = ProgrammingLanguage::PlainText;
    QString m_searchString;
    QTextDocument::FindFlags m_searchFlags;
    void updateExtraSelections();

    QFileSystemWatcher *m_fileWatcher;
    QString m_currentFilePath;
    bool m_isSaving;
};

class LineNumberArea : public QWidget {
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};