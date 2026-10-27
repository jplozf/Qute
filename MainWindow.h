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
#include "SettingsDialog.h"

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

private:
    enum { MaxRecentFiles = 5 };

    QTabWidget *tabWidget;
    FindReplaceDialog *findDialog;
    QLabel *statusLabel;
    QLabel *versionLabel;
    QFont currentEditorFont;
    EditorTheme currentTheme;
    QLabel *toolbarLangLabel;

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
};

ProgrammingLanguage detectLanguage(const QString &fileName);
QString formatByteSize(qint64 bytes);