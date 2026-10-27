#include "MainWindow.h"
#include "SyntaxHighlighter.h"
#include "CodeEditor.h"
#include <QMenuBar>
#include <QToolBar>
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
#include <QPrintDialog>
#include <QPrinter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), tabWidget(new QTabWidget(this)), findDialog(nullptr), currentTheme(EditorTheme::Light)
{
    appConstants = new Constants();

    setCentralWidget(tabWidget);
    tabWidget->setTabsClosable(true);

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);

    // Setup Status Bar
    QString aboutText = QString("Version : %1.%2-%3")
                            .arg(APP_VERSION)
                            .arg(GIT_COMMIT_COUNT)
                            .arg(GIT_HASH);
    versionLabel = new QLabel(aboutText, this);
    versionLabel->setStyleSheet("color: gray;"); // Optional: subtle styling
    statusBar()->addWidget(versionLabel);       // Left side of the status bar

    statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel); // Right side (Cursor position)

    setupMenus();
    setupToolBar();

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
    // File Menu Actions
    newAction = new QAction(tr("&New"), this);
    newAction->setShortcut(QKeySequence::New);
    newAction->setIcon(QIcon(":/icons/Document Text.png"));
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);

    openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon(QIcon(":/icons/Folder Open.png"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(QIcon(":/icons/Save.png"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);

    saveAsAction = new QAction(tr("Save &As..."), this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setIcon(QIcon(":/icons/Save.png"));
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAsFile);

    closeTabAction = new QAction(tr("&Close Tab"), this);
    closeTabAction->setShortcut(QKeySequence::Close);
    connect(closeTabAction, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    printAction = new QAction(tr("&Print..."), this);
    printAction->setShortcut(QKeySequence::Print);
    printAction->setIcon(QIcon(":/icons/Printer.png"));
    connect(printAction, &QAction::triggered, this, &MainWindow::print);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAction);
    fileMenu->addAction(openAction);

    recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActions[i] = recentFilesMenu->addAction(QString(), this, &MainWindow::openRecentFile);
        recentFileActions[i]->setVisible(false);
    }

    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(printAction);
    fileMenu->addSeparator();
    fileMenu->addAction(closeTabAction);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);

    // Edit Menu Actions
    cutAction = new QAction(tr("Cu&t"), this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &MainWindow::cut);

    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copy);

    pasteAction = new QAction(tr("&Paste"), this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::paste);

    findAction = new QAction(tr("&Find / Replace..."), this);
    findAction->setShortcut(QKeySequence::Find);
    findAction->setIcon(QIcon(":/icons/Search.png"));
    connect(findAction, &QAction::triggered, this, &MainWindow::showFindDialog);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(findAction);

    // View Menu Actions (Zoom In / Zoom Out)
    zoomInAction = new QAction(tr("Zoom &In"), this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    zoomInAction->setIcon(QIcon(":/icons/Zoom In.png"));
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);

    zoomOutAction = new QAction(tr("Zoom &Out"), this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction->setIcon(QIcon(":/icons/Zoom Out.png"));
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAction);
    viewMenu->addAction(zoomOutAction);

    // Settings Menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsAction = new QAction(tr("&Preferences ..."), this);
    settingsAction->setIcon(QIcon(":/icons/Gear.png"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    settingsMenu->addAction(settingsAction);
}

void MainWindow::zoomIn() {
    if (CodeEditor *editor = currentEditor()) {
        editor->zoomIn(1);
    }
}

void MainWindow::zoomOut() {
    if (CodeEditor *editor = currentEditor()) {
        editor->zoomOut(1);
    }
}

void MainWindow::print() {
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        editor->print(&printer);
    }
}

void MainWindow::openSettings() {
    SettingsDialog dialog(currentEditorFont, currentTheme, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentEditorFont = dialog.getSelectedFont();
        currentTheme = dialog.getSelectedTheme();
        applySettingsToAllEditors();
    }
}

void MainWindow::setupToolBar() {
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolBar->addAction(newAction);
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addAction(saveAsAction);
    toolBar->addAction(printAction);
    toolBar->addSeparator();
    toolBar->addAction(findAction);
    toolBar->addSeparator();
    toolBar->addAction(zoomInAction);
    toolBar->addAction(zoomOutAction);
    toolBar->addSeparator();
    toolBar->addAction(settingsAction);

    // --- Create an expanding spacer to push items to the right ---
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);

    // Create and embed the QLabel into the toolbar
    toolbarLangLabel = new QLabel(tr(" Language: C++ "), this);
    toolbarLangLabel->setStyleSheet("color: gray; font-weight: bold; padding: 0 4px;");
    toolBar->addWidget(toolbarLangLabel);
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
    // Calculate file size and human-readable size
    QString text = editor->toPlainText();
    qint64 byteSize = text.toUtf8().size();
    QString humanSize = formatByteSize(byteSize);

    // Get cursor and line information
    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    int totalLines = editor->document()->blockCount();

    // Calculate percentage through the file based on lines
    int percentage = 0;
    if (totalLines > 0) {
        percentage = (line * 100) / totalLines;
        if (percentage > 100) percentage = 100;
    }

    // Format status bar text cleanly
    statusLabel->setText(QString("Size: %1 (%2 bytes)  |  %3%  |  Ln: %4, Col: %5")
                             .arg(humanSize)
                             .arg(byteSize)
                             .arg(percentage)
                             .arg(line)
                             .arg(col));
}

void MainWindow::currentTabChanged(int /* index */) {
    updateCursorPosition();
    CodeEditor *editor = currentEditor();
    if (editor) {
        if (SyntaxHighlighter *hl = editor->document()->findChild<SyntaxHighlighter*>()) {
            toolbarLangLabel->setText(QString(" Language: %1 ").arg(languageToString(hl->language())));
        }
    } else {
        toolbarLangLabel->setText(tr(" Language: None "));
    }
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

void MainWindow::saveAsFile() {
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save File As"));
    if (filePath.isEmpty())
        return;

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

QString MainWindow::languageToString(ProgrammingLanguage lang) {
    switch (lang) {
    case ProgrammingLanguage::Cpp: return "C++";
    case ProgrammingLanguage::Python: return "Python";
    case ProgrammingLanguage::Go: return "Go";
    case ProgrammingLanguage::JavaScript: return "JavaScript";
    case ProgrammingLanguage::Java: return "Java";
    case ProgrammingLanguage::Html: return "HTML";
    case ProgrammingLanguage::Xml: return "XML";
    }
    return "Unknown";
}

ProgrammingLanguage detectLanguage(const QString &fileName) {
    if (fileName.endsWith(".py", Qt::CaseInsensitive))
        return ProgrammingLanguage::Python;
    if (fileName.endsWith(".go", Qt::CaseInsensitive))
        return ProgrammingLanguage::Go;
    if (fileName.endsWith(".js", Qt::CaseInsensitive) || fileName.endsWith(".ts", Qt::CaseInsensitive))
        return ProgrammingLanguage::JavaScript;
    if (fileName.endsWith(".java", Qt::CaseInsensitive))
        return ProgrammingLanguage::Java;
    if (fileName.endsWith(".html", Qt::CaseInsensitive) || fileName.endsWith(".htm", Qt::CaseInsensitive))
        return ProgrammingLanguage::Html;
    if (fileName.endsWith(".xml", Qt::CaseInsensitive) || fileName.endsWith(".svg", Qt::CaseInsensitive))
        return ProgrammingLanguage::Xml;

    return ProgrammingLanguage::Cpp; // Default
}

QString formatByteSize(qint64 bytes) {
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    else if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    else
        return QString("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
}