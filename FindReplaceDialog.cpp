#include "FindReplaceDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTextDocument>

FindReplaceDialog::FindReplaceDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent), m_editor(editor)
{
    setWindowTitle(tr("Find and Replace"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(400, 160);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Find Row
    QHBoxLayout *findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel(tr("Find what:"), this));
    findLineEdit = new QLineEdit(this);
    findLayout->addWidget(findLineEdit);
    mainLayout->addLayout(findLayout);

    // Replace Row
    QHBoxLayout *replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel(tr("Replace with:"), this));
    replaceLineEdit = new QLineEdit(this);
    replaceLayout->addWidget(replaceLineEdit);
    mainLayout->addLayout(replaceLayout);

    // Options Row
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    caseCheckBox = new QCheckBox(tr("Case sensitive"), this);
    wholeWordCheckBox = new QCheckBox(tr("Whole words"), this);
    optionsLayout->addWidget(caseCheckBox);
    optionsLayout->addWidget(wholeWordCheckBox);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    // Buttons Row
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    findButton = new QPushButton(tr("&Find Next"), this);
    replaceButton = new QPushButton(tr("&Replace"), this);
    replaceAllButton = new QPushButton(tr("Replace &All"), this);
    QPushButton *closeButton = new QPushButton(tr("Close"), this);

    buttonLayout->addWidget(findButton);
    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(replaceAllButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(findButton, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replace);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(findLineEdit, &QLineEdit::returnPressed, this, &FindReplaceDialog::findNext);
}

void FindReplaceDialog::setEditor(CodeEditor *editor) {
    m_editor = editor;
}

void FindReplaceDialog::findNext() {
    if (!m_editor) return;
    QString target = findLineEdit->text();
    if (target.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (wholeWordCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;

    bool found = m_editor->find(target, flags);
    if (!found) {
        // Wrap around to start of document
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);
        found = m_editor->find(target, flags);
        if (!found) {
            QMessageBox::information(this, tr("Find"), tr("Text not found."));
        }
    }
}

void FindReplaceDialog::replace() {
    if (!m_editor) return;
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == findLineEdit->text()) {
        cursor.insertText(replaceLineEdit->text());
        m_editor->setTextCursor(cursor);
    }
    findNext();
}

void FindReplaceDialog::replaceAll() {
    if (!m_editor) return;
    QString target = findLineEdit->text();
    QString replacement = replaceLineEdit->text();
    if (target.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (wholeWordCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    int count = 0;
    while (m_editor->find(target, flags)) {
        QTextCursor c = m_editor->textCursor();
        c.insertText(replacement);
        count++;
    }
    cursor.endEditBlock();

    QMessageBox::information(this, tr("Replace All"), tr("Replaced %1 occurrences.").arg(count));
}