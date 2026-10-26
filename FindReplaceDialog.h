#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include "CodeEditor.h"

class FindReplaceDialog : public QDialog {
    Q_OBJECT
public:
    explicit FindReplaceDialog(CodeEditor *editor, QWidget *parent = nullptr);
    void setEditor(CodeEditor *editor);

private slots:
    void findNext();
    void replace();
    void replaceAll();

private:
    CodeEditor *m_editor;
    QLineEdit *findLineEdit;
    QLineEdit *replaceLineEdit;
    QCheckBox *caseCheckBox;
    QCheckBox *wholeWordCheckBox;
    QPushButton *findButton;
    QPushButton *replaceButton;
    QPushButton *replaceAllButton;
};