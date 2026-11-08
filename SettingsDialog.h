#pragma once

#include <QDialog>
#include <QFont>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include "SyntaxHighlighter.h"
#include "MainWindow.h"

class MainWindow; // Forward declaration

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(const QFont &font, EditorTheme theme, int tabWidth,
                   bool insertSpaces, bool minimizeToTray, MainWindow *parent = nullptr);

    QFont getSelectedFont() const { return m_font; }
    EditorTheme getSelectedTheme() const;
    int getTabWidth() const;
    bool getInsertSpaces() const;
    bool getMinimizeToTray() const;

private slots:
    void changeFont();
    void openTemplatesFolder();
    void openToolsFile();
    void openConfFile();

private:
    MainWindow *m_mainWindow;
    QFont m_font;
    EditorTheme m_theme;
    int m_tabWidth;
    bool m_insertSpaces;
    bool m_minimizeToTray;

    QLabel *fontPreviewLabel;
    QComboBox *themeComboBox;
    QPushButton *fontButton;
    QSpinBox *tabWidthSpinBox;
    QCheckBox *insertSpacesCheckBox;
    QCheckBox *minimizeToTrayCheckBox;
    QPushButton *toolsFileButton;
    QPushButton *confFileButton;
    QPushButton *templatesFolderButton;
};