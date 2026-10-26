#pragma once

#include <QDialog>
#include <QFont>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "SyntaxHighlighter.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(const QFont &font, EditorTheme theme, QWidget *parent = nullptr);

    QFont getSelectedFont() const { return m_font; }
    EditorTheme getSelectedTheme() const;

private slots:
    void changeFont();

private:
    QFont m_font;
    EditorTheme m_theme;

    QLabel *fontPreviewLabel;
    QComboBox *themeComboBox;
    QPushButton *fontButton;
};