#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include "CodeEditor.h"
#include "FindReplaceDialog.h"
#include "constants.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    Constants *appConstants;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void closeCurrentTab();
    void closeTab(int index);
    void showFindDialog();
    void updateCursorPosition();
    void currentTabChanged(int index);
    void documentModified(bool modified);

    void cut();
    void copy();
    void paste();

    void nextTab();
    void previousTab();

    void chooseFont();
    void selectTheme();
    void openRecentFile();

private:
    enum { MaxRecentFiles = 5 };

    QTabWidget *tabWidget;
    FindReplaceDialog *findDialog;
    QLabel *statusLabel;
    QFont currentEditorFont;
    EditorTheme currentTheme;

    QMenu *recentFilesMenu;
    QAction *recentFileActions[MaxRecentFiles];
    QStringList recentFiles;

    // Shared Action Pointers for Menu and Toolbar
    QAction *newAction;
    QAction *openAction;
    QAction *saveAction;
    QAction *cutAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *findAction;
    QAction *closeTabAction;

    void setupMenus();
    void setupToolBar();
    void openFileWithPath(const QString &fileName);
    bool maybeSave(CodeEditor *editor);
    CodeEditor *currentEditor();
    QString strippedName(const QString &fullFileName);
    void applySettingsToAllEditors();
    void updateRecentFileActions();
    void addToRecentFiles(const QString &fileName);
};

ProgrammingLanguage detectLanguage(const QString &fileName);