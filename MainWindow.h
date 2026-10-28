#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QCloseEvent>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QToolBar>
#include <QMessageBox>
#include "CodeEditor.h"
#include "FindReplaceDialog.h"
#include "constants.h"
#include "SettingsDialog.h"

class DoubleClickLabel : public QLabel {
    Q_OBJECT
public:
    explicit DoubleClickLabel(QWidget *parent = nullptr) : QLabel(parent) {}

signals:
    void doubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        emit doubleClicked();
        QLabel::mouseDoubleClickEvent(event); // Call base implementation
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QStringList &commandLineFiles = QStringList(), QWidget *parent = nullptr);
    Constants *appConstants;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void closeCurrentTab();
    void closeTab(int index);
    void showFindDialog();
    void updateCursorPosition();
    void currentTabChanged(int index);
    void documentModified(bool modified);
    void openSettings();

    void cut();
    void copy();
    void paste();

    void nextTab();
    void previousTab();

    void openRecentFile();

    void zoomIn();
    void zoomOut();
    void print();

    void convertToSpaces();
    void convertToTabs();

private:
    enum { MaxRecentFiles = 5 };
    int currentTabWidth = 4;
    bool currentInsertSpaces = true;

    QTabWidget *tabWidget;
    FindReplaceDialog *findDialog;
    QFont currentEditorFont;
    EditorTheme currentTheme;
    DoubleClickLabel *toolbarVersionLabel;
    QLabel *langLabel;
    QLabel *sizeLabel;
    QLabel *modifiedLabel;
    QLabel *progressLabel;
    QLabel *tabPolicyLabel;
    QLabel *cursorPosLabel;

    QMenu *recentFilesMenu;
    QAction *recentFileActions[MaxRecentFiles];
    QStringList recentFiles;

    // Shared Action Pointers for Menu and Toolbar
    QAction *newAction;
    QAction *openAction;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *cutAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *findAction;
    QAction *closeTabAction;
    QAction *settingsAction;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *printAction;
    QAction *convertToSpacesAction;
    QAction *convertToTabsAction;

    void setupMenus();
    void setupToolBar();
    void openFileWithPath(const QString &fileName);
    bool maybeSave(CodeEditor *editor);
    CodeEditor *currentEditor();
    QString strippedName(const QString &fullFileName);
    void applySettingsToAllEditors();
    void updateRecentFileActions();
    void addToRecentFiles(const QString &fileName);
    QString languageToString(ProgrammingLanguage lang);
    void applySmartTabPolicy(CodeEditor *editor, ProgrammingLanguage lang);
    void showAboutMessage();
};

struct IndentInfo {
    bool usesSpaces = true;
    int width = 4;
    bool detected = false;
};

ProgrammingLanguage detectLanguage(const QString &fileName);
QString formatByteSize(qint64 bytes);
IndentInfo detectFileIndentation(const QString &content);
QString convertIndentation(const QString &content, bool targetSpaces, int targetWidth, bool sourceSpaces, int sourceWidth);

