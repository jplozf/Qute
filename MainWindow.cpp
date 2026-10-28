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
#include <QDialog>

MainWindow::MainWindow(const QStringList &commandLineFiles, QWidget *parent)
    : QMainWindow(parent), tabWidget(new QTabWidget(this)), findDialog(nullptr), currentTheme(EditorTheme::Light)
{
    appConstants = new Constants();

    setCentralWidget(tabWidget);
    tabWidget->setTabsClosable(true);

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);


    // Status Bar & Version Setup
    langLabel = new QLabel(QString("Unknown"), this);
    langLabel->setStyleSheet("color: gray;");
    statusBar()->addWidget(langLabel);

    // Create individual status widgets with fixed/minimum widths and panel borders
    sizeLabel = new QLabel(this);
    sizeLabel->setMinimumWidth(160);
    sizeLabel->setAlignment(Qt::AlignCenter);

    modifiedLabel = new QLabel(this);
    modifiedLabel->setMinimumWidth(90);
    modifiedLabel->setAlignment(Qt::AlignCenter);
    modifiedLabel->setStyleSheet("color: #d9534f; font-weight: bold;");

    progressLabel = new QLabel(this);
    progressLabel->setMinimumWidth(55);
    progressLabel->setAlignment(Qt::AlignCenter);

    tabPolicyLabel = new QLabel(this);
    tabPolicyLabel->setMinimumWidth(110);
    tabPolicyLabel->setAlignment(Qt::AlignCenter);

    cursorPosLabel = new QLabel(this);
    cursorPosLabel->setMinimumWidth(110);
    cursorPosLabel->setAlignment(Qt::AlignCenter);

    // Add them as permanent widgets on the right side of the status bar
    statusBar()->addPermanentWidget(sizeLabel);
    statusBar()->addPermanentWidget(modifiedLabel);
    statusBar()->addPermanentWidget(progressLabel);
    statusBar()->addPermanentWidget(tabPolicyLabel);
    statusBar()->addPermanentWidget(cursorPosLabel);

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
    if (settings.contains("tabWidth")) {
        currentTabWidth = settings.value("tabWidth").toInt();
    }
    if (settings.contains("insertSpaces")) {
        currentInsertSpaces = settings.value("insertSpaces").toBool();
    }

    recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFileActions();

    // --- Handle Command Line Files vs Session Restoration ---
    if (!commandLineFiles.isEmpty()) {
        for (const QString &filePath : commandLineFiles) {
            QString absPath = QFileInfo(filePath).absoluteFilePath();
            openFileWithPath(absPath); // Opens existing files or creates a new buffer if it doesn't exist yet
        }
    } else {
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
    settings.setValue("tabWidth", currentTabWidth);
    settings.setValue("insertSpaces", currentInsertSpaces);

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

    convertToSpacesAction = new QAction(tr("Convert to &Spaces"), this);
    connect(convertToSpacesAction, &QAction::triggered, this, &MainWindow::convertToSpaces);

    convertToTabsAction = new QAction(tr("Convert to &Tabs"), this);
    connect(convertToTabsAction, &QAction::triggered, this, &MainWindow::convertToTabs);

    QMenu *indentMenu = editMenu->addMenu(tr("&Indentation"));
    indentMenu->addAction(convertToSpacesAction);
    indentMenu->addAction(convertToTabsAction);

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
    CodeEditor *editor = currentEditor();

    // Grab policy from the active editor if available, otherwise fall back to globals
    int activeTabWidth = editor ? editor->tabWidth() : currentTabWidth;
    bool activeInsertSpaces = editor ? editor->insertSpacesForTabs() : currentInsertSpaces;

    SettingsDialog dialog(currentEditorFont, currentTheme, activeTabWidth, activeInsertSpaces, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentEditorFont = dialog.getSelectedFont();
        currentTheme = dialog.getSelectedTheme();
        currentTabWidth = dialog.getTabWidth();
        currentInsertSpaces = dialog.getInsertSpaces();

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

    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPix(":/icons/Qute_icon.png");
    if (!logoPix.isNull()) {
        // Scale it nicely to fit the toolbar height
        logoLabel->setPixmap(logoPix.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // Fallback if icon isn't found yet
        logoLabel->setText("⚡");
    }
    toolBar->addWidget(logoLabel);

    // Create and embed the QLabel into the toolbar
    // Setup Status Bar
    QString aboutText = QString("v%1.%2-%3")
                            .arg(APP_VERSION)
                            .arg(GIT_COMMIT_COUNT)
                            .arg(GIT_HASH);
    toolbarVersionLabel = new DoubleClickLabel(this);
    toolbarVersionLabel->setText(aboutText);
    connect(toolbarVersionLabel, &DoubleClickLabel::doubleClicked, this, [this]() {
        showAboutMessage();
    });
    toolbarVersionLabel->setStyleSheet("color: gray; padding: 0 4px;");
    toolBar->addWidget(toolbarVersionLabel);
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
            editor->setTabWidth(currentTabWidth);
            editor->setInsertSpacesForTabs(currentInsertSpaces);
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
        sizeLabel->clear();
        modifiedLabel->clear();
        progressLabel->clear();
        tabPolicyLabel->clear();
        cursorPosLabel->clear();
        return;
    }

    // 1. File size metrics
    QString text = editor->toPlainText();
    qint64 byteSize = text.toUtf8().size();
    QString humanSize = formatByteSize(byteSize);
    sizeLabel->setText(QString(" Size: %1 (%2 bytes) ").arg(humanSize).arg(byteSize));

    // 2. Modified flag
    if (editor->document()->isModified()) {
        modifiedLabel->setText("*modified*");
    } else {
        modifiedLabel->clear();
    }

    // 3. Cursor & Line metrics
    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    int totalLines = editor->document()->blockCount();

    int percentage = 0;
    if (totalLines > 0) {
        percentage = (line * 100) / totalLines;
        if (percentage > 100) percentage = 100;
    }
    progressLabel->setText(QString("%1%").arg(percentage));

    // 4. Tab policy
    QString tabPolicy = editor->insertSpacesForTabs()
                            ? QString("Spaces: %1").arg(editor->tabWidth())
                            : QString("Tab: %1").arg(editor->tabWidth());
    tabPolicyLabel->setText(tabPolicy);

    // 5. Line and Column coordinates
    cursorPosLabel->setText(QString("Ln: %1, Col: %2 ").arg(line).arg(col));
}

void MainWindow::currentTabChanged(int /* index */) {
    updateCursorPosition();
    CodeEditor *editor = currentEditor();
    if (editor) {
        if (SyntaxHighlighter *hl = editor->document()->findChild<SyntaxHighlighter*>()) {
            langLabel->setText(QString(" %1 ").arg(languageToString(hl->language())));
        }
    } else {
        langLabel->setText(tr(" Unknown "));
    }
}

void MainWindow::documentModified(bool modified) {
    if (QTextDocument *doc = qobject_cast<QTextDocument*>(sender())) {
        // Find which CodeEditor owns this document
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i))) {
                if (editor->document() == doc) {
                    QString title = tabWidget->tabText(i);
                    // Remove any trailing spaces or existing stars to get clean base name
                    title = title.trimmed();
                    if (title.endsWith('*')) {
                        title.chop(1);
                        title = title.trimmed();
                    }

                    if (modified) {
                        tabWidget->setTabText(i, title + " *");
                        modifiedLabel->setText("*modified*");
                    } else {
                        tabWidget->setTabText(i, title);
                        modifiedLabel->clear();
                    }
                    break;
                }
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
    QString content = in.readAll();
    file.close();

    CodeEditor *editor = new CodeEditor(this);
    editor->setFont(currentEditorFont);
    editor->setTheme(currentTheme);

    ProgrammingLanguage lang = detectLanguage(fileName);
    applySmartTabPolicy(editor, lang);

    // Check indentation mismatch
    IndentInfo fileIndent = detectFileIndentation(content);
    if (fileIndent.detected) {
        bool targetSpaces = editor->insertSpacesForTabs();
        int targetWidth = editor->tabWidth();

        if (fileIndent.usesSpaces != targetSpaces || (fileIndent.usesSpaces && fileIndent.width != targetWidth)) {
            QString fileStyle = fileIndent.usesSpaces ? QString("%1 spaces").arg(fileIndent.width) : "tabs";
            QString targetStyle = targetSpaces ? QString("%1 spaces").arg(targetWidth) : "tabs";

            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, tr("Indentation Mismatch"),
                                          tr("The file '%1' uses %2, but your policy for this language is %3.\n\nWould you like to apply the current policy to this file?")
                                              .arg(QFileInfo(fileName).fileName())
                                              .arg(fileStyle)
                                              .arg(targetStyle),
                                          QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                content = convertIndentation(content, targetSpaces, targetWidth, fileIndent.usesSpaces, fileIndent.width);
                editor->document()->setModified(true); // Mark modified so user knows it was changed
            }
        }
    }

    editor->setPlainText(content);

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
    updateCursorPosition();
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
    updateCursorPosition();
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
    case ProgrammingLanguage::Bash: return "Bash";
    }
    return "Unknown";
}

ProgrammingLanguage detectLanguage(const QString &fileName) {
    QFileInfo fileInfo(fileName);
    QString ext = fileInfo.suffix().toLower();
    QString name = fileInfo.fileName().toLower();

    if (ext == "cpp" || ext == "cc" || ext == "h") return ProgrammingLanguage::Cpp;
    if (ext == "py") return ProgrammingLanguage::Python;
    if (ext == "go") return ProgrammingLanguage::Go;
    if (ext == "js") return ProgrammingLanguage::JavaScript;
    if (ext == "java") return ProgrammingLanguage::Java;
    if (ext == "html" || ext == "htm") return ProgrammingLanguage::Html;
    if (ext == "xml") return ProgrammingLanguage::Xml;
    if (ext == "sh" || ext == "bash" || name.startsWith("env")) return ProgrammingLanguage::Bash;

    return ProgrammingLanguage::Cpp; // Default fallback
}

QString formatByteSize(qint64 bytes) {
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    else if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    else
        return QString("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
}

void MainWindow::applySmartTabPolicy(CodeEditor *editor, ProgrammingLanguage lang) {
    switch (lang) {
    case ProgrammingLanguage::Python:
        editor->setInsertSpacesForTabs(true);
        editor->setTabWidth(4); // PEP 8 standard
        break;
    case ProgrammingLanguage::Go:
        editor->setInsertSpacesForTabs(false); // Go standard style uses literal tabs (\t) via gofmt
        editor->setTabWidth(4);
        break;
    case ProgrammingLanguage::Html:
    case ProgrammingLanguage::Xml:
        editor->setInsertSpacesForTabs(true);
        editor->setTabWidth(2); // Common web/markup convention
        break;
    case ProgrammingLanguage::Bash:
        editor->setInsertSpacesForTabs(false);
        editor->setTabWidth(4);
        break;
    default:
        // Fall back to user's global preferences for C++, JS, Java, etc.
        editor->setInsertSpacesForTabs(currentInsertSpaces);
        editor->setTabWidth(currentTabWidth);
        break;
    }
}

// Scan the first few lines to guess if the file uses tabs or spaces and what width
IndentInfo detectFileIndentation(const QString &content) {
    QStringList lines = content.split('\n');
    int tabCount = 0;
    QMap<int, int> spaceWidthCounts;
    int checkedLines = 0;

    for (const QString &line : lines) {
        if (line.isEmpty()) continue;
        if (line.startsWith('\t')) {
            tabCount++;
        } else if (line.startsWith(' ')) {
            int spaces = 0;
            while (spaces < line.length() && line[spaces] == ' ') {
                spaces++;
            }
            if (spaces > 1 && spaces <= 8) {
                spaceWidthCounts[spaces]++;
            }
        }
        if (++checkedLines > 50) break; // Sample first 50 non-empty lines
    }

    IndentInfo info;
    if (tabCount > 3 && tabCount >= spaceWidthCounts.values().size()) {
        info.usesSpaces = false;
        info.detected = true;
    } else if (!spaceWidthCounts.isEmpty()) {
        int maxCount = 0;
        int bestWidth = 4;
        for (auto it = spaceWidthCounts.cbegin(); it != spaceWidthCounts.cend(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                bestWidth = it.key();
            }
        }
        info.usesSpaces = true;
        info.width = bestWidth;
        info.detected = true;
    }
    return info;
}

// Convert document indentation from source style to target style
QString convertIndentation(const QString &content, bool targetSpaces, int targetWidth, bool sourceSpaces, int sourceWidth) {
    QStringList lines = content.split('\n');
    QStringList convertedLines;

    for (QString line : lines) {
        int indentLevels = 0;
        int i = 0;

        if (sourceSpaces) {
            int leadingSpaces = 0;
            while (i < line.length() && line[i] == ' ') {
                leadingSpaces++;
                i++;
            }
            indentLevels = sourceWidth > 0 ? leadingSpaces / sourceWidth : 0;
        } else {
            int leadingTabs = 0;
            while (i < line.length() && line[i] == '\t') {
                leadingTabs++;
                i++;
            }
            indentLevels = leadingTabs;
        }

        QString newIndent = targetSpaces ? QString(indentLevels * targetWidth, ' ') : QString(indentLevels, '\t');
        convertedLines.append(newIndent + line.mid(i));
    }
    return convertedLines.join('\n');
}

void MainWindow::convertToSpaces() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    QString content = editor->toPlainText();
    IndentInfo currentIndent = detectFileIndentation(content);

    bool sourceSpaces = currentIndent.detected ? currentIndent.usesSpaces : !editor->insertSpacesForTabs();
    int sourceWidth = currentIndent.detected ? currentIndent.width : editor->tabWidth();
    int targetWidth = editor->tabWidth();

    QString converted = convertIndentation(content, true, targetWidth, sourceSpaces, sourceWidth);

    editor->setPlainText(converted);
    editor->setInsertSpacesForTabs(true);
    editor->document()->setModified(true);
}

void MainWindow::convertToTabs() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    QString content = editor->toPlainText();
    IndentInfo currentIndent = detectFileIndentation(content);

    bool sourceSpaces = currentIndent.detected ? currentIndent.usesSpaces : !editor->insertSpacesForTabs();
    int sourceWidth = currentIndent.detected ? currentIndent.width : editor->tabWidth();
    int targetWidth = editor->tabWidth();

    QString converted = convertIndentation(content, false, targetWidth, sourceSpaces, sourceWidth);

    editor->setPlainText(converted);
    editor->setInsertSpacesForTabs(false);
    editor->document()->setModified(true);
}

void MainWindow::showAboutMessage() {
    QMessageBox msgBox(this);
    QString aboutText = QString("%1 v%2.%3-%4")
                            .arg(appConstants->getQString("APPLICATION_NAME"))
                            .arg(APP_VERSION)
                            .arg(GIT_COMMIT_COUNT)
                            .arg(GIT_HASH);
    msgBox.setWindowTitle("About " + appConstants->getQString("APPLICATION_NAME"));
    msgBox.setText(aboutText + "\n\n" + appConstants->getQString("COPYRIGHT") + "\n\n" + appConstants->getQString("EMAIL"));

    QPixmap customIcon(":/icons/Qute_icon.png");
    if (!customIcon.isNull()) {
        msgBox.setIconPixmap(customIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}