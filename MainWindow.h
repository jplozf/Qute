#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include "CodeEditor.h"
#include "FindReplaceDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

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

    void setupMenus();
    void openFileWithPath(const QString &fileName);
    bool maybeSave(CodeEditor *editor);
    CodeEditor *currentEditor();
    QString strippedName(const QString &fullFileName);
    void applySettingsToAllEditors();
    void updateRecentFileActions();
    void addToRecentFiles(const QString &fileName);
};