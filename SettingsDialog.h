#pragma once

#include <QDialog>
#include <QFont>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include "SyntaxHighlighter.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(const QFont &font, EditorTheme theme, int tabWidth, bool insertSpaces, QWidget *parent = nullptr);

    QFont getSelectedFont() const { return m_font; }
    EditorTheme getSelectedTheme() const;
    int getTabWidth() const;
    bool getInsertSpaces() const;

private slots:
    void changeFont();

private:
    QFont m_font;
    EditorTheme m_theme;
    int m_tabWidth;
    bool m_insertSpaces;

    QLabel *fontPreviewLabel;
    QComboBox *themeComboBox;
    QPushButton *fontButton;
    QSpinBox *tabWidthSpinBox;
    QCheckBox *insertSpacesCheckBox;
};