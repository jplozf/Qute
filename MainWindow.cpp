#include "MainWindow.h"
#include "SyntaxHighlighter.h"
#include "CodeEditor.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextCursor>
#include <QSettings>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), tabWidget(new QTabWidget(this)), findDialog(nullptr), currentTheme(EditorTheme::Light)
{
    appConstants = new Constants();

    setCentralWidget(tabWidget);
    tabWidget->setTabsClosable(true);

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);

    // Setup Status Bar
    statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel);

    setupMenus();

    // Global keyboard shortcuts for tab navigation
    // new QShortcut(QKeySystemShortcut(QKeySequence::NextChild), this, SLOT(nextTab()));
    // new QShortcut(QKeySystemShortcut(QKeySequence::PreviousChild), this, SLOT(previousTab()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown), this, SLOT(nextTab()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageUp), this, SLOT(previousTab()));

    resize(900, 700);
    setWindowTitle(appConstants->getQString("APPLICATION_NAME"));

    // Default Font
    currentEditorFont = QFont("Monospace", 11);
    currentEditorFont.setStyleHint(QFont::TypeWriter);

    // Restore Session Settings & Preferences
    QSettings settings(appConstants->getQString("ORGANIZATION_NAME"), appConstants->getQString("APPLICATION_NAME"));
    restoreGeometry(settings.value("geometry").toByteArray());

    if (settings.contains("font")) {
        currentEditorFont = settings.value("font").value<QFont>();
    }
    if (settings.contains("theme")) {
        currentTheme = static_cast<EditorTheme>(settings.value("theme").toInt());
    }

    recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFileActions();

    QStringList openFiles = settings.value("openFiles").toStringList();
    int currentIndex = settings.value("currentIndex", 0).toInt();

    if (!openFiles.isEmpty()) {
        for (const QString &file : openFiles) {
            if (QFile::exists(file)) {
                openFileWithPath(file);
            }
        }
        if (tabWidget->count() > 0) {
            tabWidget->setCurrentIndex(qMin(currentIndex, tabWidget->count() - 1));
        }
    }

    if (tabWidget->count() == 0) {
        newFile();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor && editor->document()->isModified()) {
            tabWidget->setCurrentIndex(i);
            if (!maybeSave(editor)) {
                event->ignore();
                return;
            }
        }
    }

    QSettings settings(appConstants->getQString("ORGANIZATION_NAME"), appConstants->getQString("APPLICATION_NAME"));
    settings.setValue("geometry", saveGeometry());
    settings.setValue("font", currentEditorFont);
    settings.setValue("theme", static_cast<int>(currentTheme));
    settings.setValue("recentFiles", recentFiles);

    QStringList openFiles;
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        QString filePath = tabWidget->tabToolTip(i);
        if (!filePath.isEmpty() && QFile::exists(filePath)) {
            openFiles.append(filePath);
        }
    }
    settings.setValue("openFiles", openFiles);
    settings.setValue("currentIndex", tabWidget->currentIndex());

    event->accept();
}

void MainWindow::setupMenus() {
    // File Menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), this, &MainWindow::newFile, QKeySequence::New);
    fileMenu->addAction(tr("&Open..."), this, &MainWindow::openFile, QKeySequence::Open);

    // Recent Files Submenu
    recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActions[i] = recentFilesMenu->addAction(QString(), this, &MainWindow::openRecentFile);
        recentFileActions[i]->setVisible(false);
    }

    fileMenu->addAction(tr("&Save"), this, &MainWindow::saveFile, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Close Tab"), this, &MainWindow::closeCurrentTab, QKeySequence::Close);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);

    // Edit Menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("Cu&t"), this, &MainWindow::cut, QKeySequence::Cut);
    editMenu->addAction(tr("&Copy"), this, &MainWindow::copy, QKeySequence::Copy);
    editMenu->addAction(tr("&Paste"), this, &MainWindow::paste, QKeySequence::Paste);
    editMenu->addSeparator();
    editMenu->addAction(tr("&Find / Replace..."), this, &MainWindow::showFindDialog, QKeySequence::Find);

    // Settings Menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(tr("Choose &Font..."), this, &MainWindow::chooseFont);
    settingsMenu->addAction(tr("Select &Theme..."), this, &MainWindow::selectTheme);
}

void MainWindow::addToRecentFiles(const QString &fileName) {
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    while (recentFiles.size() > MaxRecentFiles)
        recentFiles.removeLast();
    updateRecentFileActions();
}

void MainWindow::updateRecentFileActions() {
    QMutableStringListIterator i(recentFiles);
    while (i.hasNext()) {
        if (!QFile::exists(i.next()))
            i.remove();
    }

    int numRecentFiles = qMin(recentFiles.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(strippedName(recentFiles[i]));
        recentFileActions[i]->setText(text);
        recentFileActions[i]->setData(recentFiles[i]);
        recentFileActions[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j) {
        recentFileActions[j]->setVisible(false);
    }
    recentFilesMenu->setEnabled(numRecentFiles > 0);
}

void MainWindow::openRecentFile() {
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QString fileName = action->data().toString();
        if (QFile::exists(fileName)) {
            for (int i = 0; i < tabWidget->count(); ++i) {
                if (tabWidget->tabToolTip(i) == fileName) {
                    tabWidget->setCurrentIndex(i);
                    return;
                }
            }
            openFileWithPath(fileName);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("File %1 does not exist.").arg(fileName));
            recentFiles.removeAll(fileName);
            updateRecentFileActions();
        }
    }
}

void MainWindow::chooseFont() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, currentEditorFont, this, tr("Choose Monospace Font"));
    if (ok) {
        currentEditorFont = font;
        applySettingsToAllEditors();
    }
}

void MainWindow::selectTheme() {
    QStringList themes = {
        tr("Light"),
        tr("Dark (VS Code)"),
        tr("Monokai"),
        tr("Solarized Dark"),
        tr("Nord")
    };
    int currentIndex = static_cast<int>(currentTheme);

    bool ok;
    QString item = QInputDialog::getItem(this, tr("Select Syntax Theme"), tr("Theme:"), themes, currentIndex, false, &ok);
    if (ok && !item.isEmpty()) {
        if (item == themes[0]) currentTheme = EditorTheme::Light;
        else if (item == themes[1]) currentTheme = EditorTheme::DarkVS;
        else if (item == themes[2]) currentTheme = EditorTheme::Monokai;
        else if (item == themes[3]) currentTheme = EditorTheme::SolarizedDark;
        else if (item == themes[4]) currentTheme = EditorTheme::Nord;

        applySettingsToAllEditors();
    }
}

void MainWindow::applySettingsToAllEditors() {
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i))) {
            editor->setFont(currentEditorFont);
            editor->setTheme(currentTheme);
            if (SyntaxHighlighter *hl = editor->document()->findChild<SyntaxHighlighter*>()) {
                hl->setTheme(currentTheme);
            }
        }
    }
}

CodeEditor *MainWindow::currentEditor() {
    if (QWidget *widget = tabWidget->currentWidget()) {
        return qobject_cast<CodeEditor*>(widget);
    }
    return nullptr;
}

QString MainWindow::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::showFindDialog() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    if (!findDialog) {
        findDialog = new FindReplaceDialog(editor, this);
    } else {
        findDialog->setEditor(editor);
    }
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void MainWindow::updateCursorPosition() {
    CodeEditor *editor = currentEditor();
    if (!editor) {
        statusLabel->clear();
        return;
    }
    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    statusLabel->setText(QString("Ln: %1, Col: %2").arg(line).arg(col));
}

void MainWindow::currentTabChanged(int /* index */) {
    updateCursorPosition();
}

void MainWindow::documentModified(bool modified) {
    if (CodeEditor *editor = qobject_cast<CodeEditor*>(sender())) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            QString title = tabWidget->tabText(index);
            if (modified && !title.endsWith("*")) {
                tabWidget->setTabText(index, title + "*");
            } else if (!modified && title.endsWith("*")) {
                title.chop(1);
                tabWidget->setTabText(index, title);
            }
        }
    }
}

void MainWindow::cut() {
    if (CodeEditor *editor = currentEditor()) editor->cut();
}

void MainWindow::copy() {
    if (CodeEditor *editor = currentEditor()) editor->copy();
}

void MainWindow::paste() {
    if (CodeEditor *editor = currentEditor()) editor->paste();
}

void MainWindow::nextTab() {
    int count = tabWidget->count();
    if (count > 1) {
        int nextIndex = (tabWidget->currentIndex() + 1) % count;
        tabWidget->setCurrentIndex(nextIndex);
    }
}

void MainWindow::previousTab() {
    int count = tabWidget->count();
    if (count > 1) {
        int prevIndex = (tabWidget->currentIndex() - 1 + count) % count;
        tabWidget->setCurrentIndex(prevIndex);
    }
}

void MainWindow::newFile() {
    CodeEditor *editor = new CodeEditor(this);
    editor->setFont(currentEditorFont);
    editor->setTheme(currentTheme);

    SyntaxHighlighter *hl = new SyntaxHighlighter(editor->document());
    hl->setTheme(currentTheme);

    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::documentModified);

    int index = tabWidget->addTab(editor, tr("Untitled*"));
    tabWidget->setCurrentIndex(index);
    updateCursorPosition();
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
    if (!fileName.isEmpty()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabToolTip(i) == fileName) {
                tabWidget->setCurrentIndex(i);
                return;
            }
        }
        openFileWithPath(fileName);
    }
}

void MainWindow::openFileWithPath(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    QTextStream in(&file);
    CodeEditor *editor = new CodeEditor(this);
    editor->setFont(currentEditorFont);
    editor->setTheme(currentTheme);

    editor->setPlainText(in.readAll());
    SyntaxHighlighter *hl = new SyntaxHighlighter(editor->document());
    hl->setLanguage(detectLanguage(fileName));
    hl->setTheme(currentTheme);

    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::documentModified);

    int index = tabWidget->addTab(editor, strippedName(fileName));
    tabWidget->setTabToolTip(index, fileName);
    tabWidget->setCurrentIndex(index);
    updateCursorPosition();

    addToRecentFiles(fileName);
}

void MainWindow::saveFile() {
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    QString filePath = tabWidget->tabToolTip(tabWidget->currentIndex());
    if (filePath.isEmpty() || !QFileInfo(filePath).exists()) {
        filePath = QFileDialog::getSaveFileName(this, tr("Save File"));
        if (filePath.isEmpty())
            return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot write file %1:\n%2.").arg(filePath, file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << editor->toPlainText();
    editor->document()->setModified(false);

    int index = tabWidget->currentIndex();
    tabWidget->setTabText(index, strippedName(filePath));
    tabWidget->setTabToolTip(index, filePath);

    addToRecentFiles(filePath);
}

bool MainWindow::maybeSave(CodeEditor *editor) {
    if (!editor->document()->isModified())
        return true;

    int index = tabWidget->indexOf(editor);
    QString tabName = tabWidget->tabText(index);
    tabName.remove('*');

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Qt Text Editor"),
                               tr("The document '%1' has been modified.\nDo you want to save your changes?")
                                   .arg(tabName),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        saveFile();
        return !editor->document()->isModified();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void MainWindow::closeCurrentTab() {
    closeTab(tabWidget->currentIndex());
}

void MainWindow::closeTab(int index) {
    if (index < 0 || index >= tabWidget->count()) return;

    CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
    if (editor && editor->document()->isModified()) {
        tabWidget->setCurrentIndex(index);
        if (!maybeSave(editor)) {
            return;
        }
    }

    QWidget *widget = tabWidget->widget(index);
    tabWidget->removeTab(index);
    delete widget;

    if (tabWidget->count() == 0) {
        newFile();
    }
}

ProgrammingLanguage detectLanguage(const QString &fileName) {
    if (fileName.endsWith(".py", Qt::CaseInsensitive))
        return ProgrammingLanguage::Python;
    if (fileName.endsWith(".go", Qt::CaseInsensitive))
        return ProgrammingLanguage::Go;
    if (fileName.endsWith(".js", Qt::CaseInsensitive) || fileName.endsWith(".ts", Qt::CaseInsensitive))
        return ProgrammingLanguage::JavaScript;

    // Default to C++ for .cpp, .h, .c, .cc, or unknown extensions
    return ProgrammingLanguage::Cpp;
}