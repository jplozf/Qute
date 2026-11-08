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
    m_outlineUpdateTimer = new QTimer(this);
    m_outlineUpdateTimer->setSingleShot(true);
    m_outlineUpdateTimer->setInterval(400); // Wait 400ms after last keystroke

    connect(m_outlineUpdateTimer, &QTimer::timeout, this, &MainWindow::updateFunctionOutline);

    appConstants = new Constants();

    setCentralWidget(tabWidget);
    tabWidget->setTabsClosable(true);

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);

    // Contextual menu for tabs
    QTabBar *tabBar = tabWidget->tabBar();
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabBar, &QTabBar::customContextMenuRequested, this, [this, tabBar](const QPoint &pos) {
        // Find which tab was right-clicked based on local coordinates
        int index = tabBar->tabAt(pos);

        QMenu tabMenu(this);
        QAction *closeAction = tabMenu.addAction(tr("Close Tab"));
        QAction *closeOthersAction = tabMenu.addAction(tr("Close Other Tabs"));
        QAction *copyPathAction = tabMenu.addAction(tr("Copy File Path"));
        tabMenu.addSeparator();
        QAction *addToFavAction = tabMenu.addAction(tr("Add to Favorites"));
        tabMenu.addSeparator();
        QAction *closeAllAction = tabMenu.addAction(tr("Close All Tabs"));
        QAction *closeOutputsAction = tabMenu.addAction(tr("Close All Output Tabs"));
        connect(closeOutputsAction, &QAction::triggered, this, &MainWindow::closeAllOutputTabs);

        // Execute menu at cursor position
        QAction *selectedAction = tabMenu.exec(QCursor::pos());
        if (!selectedAction) return;

        // Capture the target widget pointer safely before the event loop ends
        QWidget *targetWidget = tabWidget->widget(index);

        // Defer execution slightly to let the menu event loop fully exit first
        QTimer::singleShot(0, this, [this, targetWidget, selectedAction, closeAction, closeOthersAction, copyPathAction, addToFavAction, closeAllAction]() {
            if (selectedAction == closeAction) {
                int idx = tabWidget->indexOf(targetWidget);
                if (idx != -1) {
                    closeTab(idx);
                }
            }
            else if (selectedAction == closeOthersAction) {
                // Close all tabs except the target, iterating backwards safely
                for (int i = tabWidget->count() - 1; i >= 0; --i) {
                    QWidget *w = tabWidget->widget(i);
                    if (w != targetWidget) {
                        int idx = tabWidget->indexOf(w);
                        if (idx != -1) {
                            closeTab(idx);
                        }
                    }
                }
            }
            else if (selectedAction == copyPathAction) {
                CodeEditor *editor = qobject_cast<CodeEditor*>(targetWidget);
                if (editor) {
                    QString path = editor->property("filePath").toString();
                    if (!path.isEmpty()) {
                        QApplication::clipboard()->setText(path);
                    }
                }
            }
            else if (selectedAction == closeAllAction) {
                // Close tabs from right to left, stopping if the user cancels an unsaved prompt
                for (int i = tabWidget->count() - 1; i >= 0; --i) {
                    int initialCount = tabWidget->count();
                    closeTab(i);

                    // If the tab count didn't change, it means the user clicked "Cancel"
                    // on an unsaved changes prompt, so we abort closing the rest.
                    if (tabWidget->count() == initialCount) {
                        break;
                    }
                }
            }
            else if (selectedAction == addToFavAction) {
                CodeEditor *editor = qobject_cast<CodeEditor*>(targetWidget);
                if (editor) {
                    QString path = editor->property("filePath").toString();
                    bool isFollow = editor->property("isFollowMode").toBool();
                    if (!path.isEmpty()) {
                        addToFavorites(path, isFollow);
                    } else {
                        QMessageBox::warning(this, tr("Favorites"), tr("Please save the file before adding it to favorites."));
                    }
                }
            }
        });
    });

    // Status Bar
    pathLabel = new QLabel(QString(""), this);
    pathLabel->setStyleSheet("color: gray;");
    statusBar()->addWidget(pathLabel);

    // Create individual status widgets with fixed/minimum widths and panel borders
    sizeLabel = new QLabel(this);
    sizeLabel->setMinimumWidth(160);
    sizeLabel->setAlignment(Qt::AlignCenter);

    modifiedLabel = new QLabel(this);
    modifiedLabel->setMinimumWidth(90);
    modifiedLabel->setAlignment(Qt::AlignCenter);
    modifiedLabel->setStyleSheet("color: #d9534f; font-weight: bold;");

    // Create the Read/Write status indicator label
    readWriteLabel = new QLabel(this);
    readWriteLabel->setMinimumWidth(50);
    readWriteLabel->setAlignment(Qt::AlignCenter);

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
    statusBar()->addPermanentWidget(readWriteLabel);
    statusBar()->addPermanentWidget(progressLabel);
    statusBar()->addPermanentWidget(tabPolicyLabel);
    statusBar()->addPermanentWidget(cursorPosLabel);

    setupMenus();
    setupFavoritesMenu();
    setupToolBar();
    setupSystemTray();

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
    if (settings.contains("minimizeToTray")) {
        currentMinimizeToTray = settings.value("minimizeToTray").toBool();
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
        int size = settings.beginReadArray("OpenFiles");
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            QString filePath = settings.value("filePath").toString();
            int langInt = settings.value("language", static_cast<int>(ProgrammingLanguage::PlainText)).toInt();
            bool isFollow = settings.value("isFollowMode", false).toBool();

            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                if (isFollow) {
                    openFileInFollowMode(filePath);
                } else {
                    // Open your file (which creates the editor tab)
                    openFileWithPath(filePath);
                }
                // Get the newly created editor and apply the saved language
                CodeEditor *editor = currentEditor();
                if (editor) {
                    ProgrammingLanguage lang = static_cast<ProgrammingLanguage>(langInt);
                    editor->setLanguage(lang);

                    // Update highlighter
                    SyntaxHighlighter *hl = editor->findChild<SyntaxHighlighter*>();
                    if (hl) {
                        hl->setLanguage(lang);
                        hl->rehighlight();
                    }
                    currentTabChanged(0); // to refresh the Language label
                }
            }
        }
        settings.endArray();
    }

    if (tabWidget->count() == 0) {
        newFile();
    }

    // Trigger update check asynchronously 2 seconds after launch
    QTimer::singleShot(2000, this, &MainWindow::checkForUpdates);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (currentMinimizeToTray && trayIcon && trayIcon->isVisible() && !m_forceQuit) {
        hide();
        event->ignore(); // Prevents application exit

        trayIcon->showMessage(
            tr("Qute"),
            tr("Application minimized to system tray. Click the tray icon to restore."),
            QIcon(":/icons/Qute_icon.png"),
            2000
            );
    } else {
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (editor && editor->document()->isModified()) {
                tabWidget->setCurrentIndex(i);
                if (!maybeSave(editor)) {
                    m_forceQuit = false;
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
        settings.setValue("minimizeToTray", currentMinimizeToTray);

        settings.beginWriteArray("OpenFiles");
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (!editor) continue;

            QString filePath = editor->property("filePath").toString();
            // Skip untitled/unsaved new tabs if you don't persist them
            if (filePath.isEmpty()) continue;

            settings.setArrayIndex(i);
            settings.setValue("filePath", filePath);
            settings.setValue("language", static_cast<int>(editor->language())); // Save language enum as int
            settings.setValue("isFollowMode", editor->property("isFollowMode").toBool());
        }
        settings.endArray();
        settings.setValue("currentIndex", tabWidget->currentIndex());

        event->accept();
        qApp->quit();
    }
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
    setupTemplatesMenu(fileMenu);
    fileMenu->addAction(openAction);

    recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActions[i] = recentFilesMenu->addAction(QString(), this, &MainWindow::openRecentFile);
        recentFileActions[i]->setVisible(false);
    }

    followAction = new QAction(tr("Open in &Follow mode ..."), this);
    followAction->setIcon(QIcon(":/icons/Recycle.png"));
    connect(followAction, &QAction::triggered, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open File for Following"));
        if (!filePath.isEmpty()) {
            openFileInFollowMode(filePath);
        }
    });
    fileMenu->addAction(followAction);

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

    QAction *goToLineAction = new QAction(tr("Go to &Line..."), this);
    goToLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    connect(goToLineAction, &QAction::triggered, this, &MainWindow::goToLine);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(findAction);
    editMenu->addAction(goToLineAction);

    convertToSpacesAction = new QAction(tr("Convert to &Spaces"), this);
    connect(convertToSpacesAction, &QAction::triggered, this, &MainWindow::convertToSpaces);

    convertToTabsAction = new QAction(tr("Convert to &Tabs"), this);
    connect(convertToTabsAction, &QAction::triggered, this, &MainWindow::convertToTabs);

    QMenu *indentMenu = editMenu->addMenu(tr("&Indentation"));
    indentMenu->addAction(convertToSpacesAction);
    indentMenu->addAction(convertToTabsAction);

    QMenu *languageMenu = menuBar()->addMenu(tr("&Language"));

    // Create action group so only one language is checked at a time
    QActionGroup *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    auto addLangAction = [&](const QString &name, ProgrammingLanguage lang) {
        QAction *action = new QAction(name, this);
        action->setCheckable(true);
        langGroup->addAction(action);
        languageMenu->addAction(action);

        connect(action, &QAction::triggered, this, [this, lang]() {
            CodeEditor *editor = currentEditor();
            if (!editor) return;

            editor->setLanguage(lang);

            // Re-apply highlighter with the new language
            SyntaxHighlighter *hl = editor->findChild<SyntaxHighlighter*>();
            if (hl) {
                hl->setLanguage(lang);
                hl->rehighlight();
            }

            currentTabChanged(0); // just to refresh the language label
        });
    };

    addLangAction(tr("Plain Text"), ProgrammingLanguage::PlainText);
    addLangAction(tr("C / C++"), ProgrammingLanguage::Cpp);
    addLangAction(tr("Python"), ProgrammingLanguage::Python);
    addLangAction(tr("Go"), ProgrammingLanguage::Go);
    addLangAction(tr("Bash"), ProgrammingLanguage::Bash);
    addLangAction(tr("JavaScript"), ProgrammingLanguage::JavaScript);
    addLangAction(tr("Java"), ProgrammingLanguage::Java);
    addLangAction(tr("HTML"), ProgrammingLanguage::Html);
    addLangAction(tr("XML"), ProgrammingLanguage::Xml);

    // Tools Menu
    setupToolsMenu();

    // Git Menu
    setupGitMenu();

    // View Menu Actions (Zoom In / Zoom Out)
    zoomInAction = new QAction(tr("Zoom &In"), this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    zoomInAction->setIcon(QIcon(":/icons/Zoom In.png"));
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);

    zoomOutAction = new QAction(tr("Zoom &Out"), this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction->setIcon(QIcon(":/icons/Zoom Out.png"));
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    // Full Screen Action
    QAction *fullScreenAction = new QAction(tr("Toggle Full Screen"), this);
    fullScreenAction->setShortcut(QKeySequence::FullScreen); // Automatically binds to F11
    connect(fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);
    addAction(fullScreenAction); // Ensures the shortcut works globally in the window

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAction);
    viewMenu->addAction(zoomOutAction);
    viewMenu->addAction(fullScreenAction);

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
    bool activeMinimizeToTray = editor ? editor->minimizeToTray() : currentMinimizeToTray;

    SettingsDialog dialog(currentEditorFont, currentTheme, activeTabWidth, activeInsertSpaces, activeMinimizeToTray, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentEditorFont = dialog.getSelectedFont();
        currentTheme = dialog.getSelectedTheme();
        currentTabWidth = dialog.getTabWidth();
        currentInsertSpaces = dialog.getInsertSpaces();
        currentMinimizeToTray = dialog.getMinimizeToTray();

        applySettingsToAllEditors();
    }
}

void MainWindow::setupToolBar() {
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolBar->addAction(newAction);
    toolBar->addAction(openAction);
    toolBar->addAction(followAction);
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

    langLabel = new QLabel(QString(" Plain Text "), this);
    langLabel->setStyleSheet("color: gray;");
    toolBar->addWidget(langLabel);

    m_functionComboBox = new QComboBox(this);
    m_functionComboBox->setMinimumWidth(200);
    m_functionComboBox->setPlaceholderText(tr("Outline..."));
    toolBar->addWidget(m_functionComboBox);

    // Connect selection signal to jump function
    connect(m_functionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::jumpToSelectedFunction);

    QWidget *gap = new QWidget(this);
    gap->setFixedWidth(10); // Adjust this value to make the gap larger or smaller
    toolBar->addWidget(gap);

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
        langLabel->setText(tr(" Plain Text "));
    }
    pathLabel->setText(QString(" %1").arg(editor->property("filePath").toString()));
    updateStatusBarReadWrite();
    updateTrayMenu();
    updateFunctionOutline();
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
    connect(editor, &QPlainTextEdit::textChanged, this, [this]() {
        m_outlineUpdateTimer->start(); // Restarts the countdown on every keystroke
    });

    int index = tabWidget->addTab(editor, tr("Untitled*"));
    tabWidget->setCurrentIndex(index);
    updateCursorPosition();
    updateTrayMenu();
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
    updateTrayMenu();
}

void MainWindow::openFileWithPath(const QString &fileName) {
    if (activateTabIfAlreadyOpen(fileName)) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    pathLabel->setText(QString(" %1").arg(fileName));

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    CodeEditor *editor = new CodeEditor(this);
    editor->setFont(currentEditorFont);
    editor->setTheme(currentTheme);

    // Detect language ONCE and store it in a local variable
    ProgrammingLanguage lang = detectLanguage(fileName);
    editor->setLanguage(lang); // Ensure editor stores the language enum

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
                editor->document()->setModified(true);
            }
        }
    }

    editor->setPlainText(content);
    editor->setProperty("filePath", fileName);

    // Pass the exact same 'lang' variable to the highlighter
    SyntaxHighlighter *hl = new SyntaxHighlighter(editor->document());
    hl->setLanguage(lang);
    hl->setTheme(currentTheme);

    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::documentModified);
    connect(editor, &QPlainTextEdit::textChanged, this, [this]() {
        m_outlineUpdateTimer->start(); // Restarts the countdown on every keystroke
    });

    int index = tabWidget->addTab(editor, strippedName(fileName));
    tabWidget->setTabToolTip(index, fileName);
    tabWidget->setCurrentIndex(index);
    updateCursorPosition();

    addToRecentFiles(fileName);
    updateTrayMenu();
    updateFunctionOutline();
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
    editor->setProperty("filePath", filePath);

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
    ret = QMessageBox::warning(this, appConstants->getQString("APPLICATION_NAME"),
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
    updateTrayMenu();
}

void MainWindow::closeTab(int index) {
    if (index < 0 || index >= tabWidget->count()) return;

    CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
    if (!editor->isReadOnly() && editor->document()->isModified()) {
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
    updateTrayMenu();
}

QString MainWindow::languageToString(ProgrammingLanguage lang) {
    switch (lang) {
    case ProgrammingLanguage::PlainText: return "Plain Text";
    case ProgrammingLanguage::Cpp: return "C++";
    case ProgrammingLanguage::Python: return "Python";
    case ProgrammingLanguage::Go: return "Go";
    case ProgrammingLanguage::JavaScript: return "JavaScript";
    case ProgrammingLanguage::Java: return "Java";
    case ProgrammingLanguage::Html: return "HTML";
    case ProgrammingLanguage::Xml: return "XML";
    case ProgrammingLanguage::Bash: return "Bash";
    }
    return "Plain Text";
}

ProgrammingLanguage detectLanguage(const QString &fileName) {
    QFileInfo fileInfo(fileName);
    QString ext = fileInfo.suffix().toLower();
    QString name = fileInfo.fileName().toLower();

    if (ext.isEmpty()) return ProgrammingLanguage::PlainText;

    // Explicit text/log extensions
    if (ext == "log" || ext == "txt" || ext == "out" || ext == "conf" || ext == "ini" || ext == "json" || ext == "md") {
        return ProgrammingLanguage::PlainText;
    }
    if (ext == "cpp" || ext == "hpp" || ext == "cc" || ext == "c" || ext == "h") return ProgrammingLanguage::Cpp;
    if (ext == "py") return ProgrammingLanguage::Python;
    if (ext == "go") return ProgrammingLanguage::Go;
    if (ext == "js") return ProgrammingLanguage::JavaScript;
    if (ext == "java") return ProgrammingLanguage::Java;
    if (ext == "html" || ext == "htm") return ProgrammingLanguage::Html;
    if (ext == "xml") return ProgrammingLanguage::Xml;
    if (ext == "sh" || ext == "bash" || name.startsWith("env")) return ProgrammingLanguage::Bash;

    return ProgrammingLanguage::PlainText; // Default fallback
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
    msgBox.setText(aboutText + "\n" + appConstants->getQString("COPYRIGHT") + "\n\n" + appConstants->getQString("EMAIL"));

    QPixmap customIcon(":/icons/Qute_icon.png");
    if (!customIcon.isNull()) {
        msgBox.setIconPixmap(customIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::setupToolsMenu() {
    // Create the Tools menu if it doesn't exist yet
    if (!toolsMenu) {
        toolsMenu = menuBar()->addMenu(tr("&Tools"));
    } else {
        toolsMenu->clear();
    }

    // Determine path: ~/.config/YourOrganization/Qute/tools.conf
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + "/" + appConstants->getQString("ORGANIZATION_NAME")
                        + "/" + appConstants->getQString("APPLICATION_NAME");
    QDir().mkpath(configDir); // Ensure directory exists
    QString confPath = configDir + "/tools.conf";

    // Create default tools.conf if it doesn't exist
    if (!QFile::exists(confPath)) {
        QFile file(confPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Define custom tools here: Menu Text : command\n";
            out << "# Placeholders: %F (full path), %f (filename), %d (directory), %n (name without ext)\n";
            out << "File Info : exiftool %F\n";
            out << "Open Containing Folder : xdg-open %d\n";
        }
    }

    // --- SETUP FILE SYSTEM WATCHER FOR LIVE RELOADING ---
    if (!toolsWatcher) {
        toolsWatcher = new QFileSystemWatcher(this);
        toolsWatcher->addPath(confPath);
        connect(toolsWatcher, &QFileSystemWatcher::fileChanged, this, [this, confPath](const QString &path) {
            // Re-add path in case file replacement on save dropped the watch
            if (!toolsWatcher->files().contains(path)) {
                toolsWatcher->addPath(path);
            }
            // Refresh the tools menu live!
            setupToolsMenu();
        });
    } else if (!toolsWatcher->files().contains(confPath)) {
        toolsWatcher->addPath(confPath);
    }

    // Read tools.conf and build menu items
    QFile file(confPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;

            int colonIdx = line.indexOf(':');
            if (colonIdx == -1) continue;

            QString menuText = line.left(colonIdx).trimmed();
            QString command = line.mid(colonIdx + 1).trimmed();

            QAction *action = toolsMenu->addAction(menuText);
            connect(action, &QAction::triggered, this, [this, command]() {
                executeToolCommand(command);
            });
        }
    }

    toolsMenu->addSeparator();

    // Add a shortcut action to edit the tools.conf file directly in Qute!
    QAction *editConfAction = toolsMenu->addAction(tr("Edit tools.conf..."));
    editConfAction->setIcon(QIcon(":/icons/Write.png"));
    connect(editConfAction, &QAction::triggered, this, [this, confPath]() {
        openFileWithPath(confPath);
    });
}

void MainWindow::executeToolCommand(const QString &commandTemplate) {
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    QString filePath = editor->property("filePath").toString();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Tools"), tr("Please save the file before running external tools."));
        return;
    }

    QFileInfo fileInfo(filePath);
    QString absPath = fileInfo.absoluteFilePath();
    QString fileName = fileInfo.fileName();
    QString dirPath = fileInfo.absolutePath();
    QString baseName = fileInfo.completeBaseName();

    // Replace placeholders
    QString cmd = commandTemplate;
    cmd.replace("%F", QString("\"%1\"").arg(absPath));
    cmd.replace("%f", QString("\"%1\"").arg(fileName));
    cmd.replace("%d", QString("\"%1\"").arg(dirPath));
    cmd.replace("%n", QString("\"%1\"").arg(baseName));

    // Create a QProcess and a high-precision timer
    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    QElapsedTimer *timer = new QElapsedTimer();
    timer->start();

    // Connect to when the process finishes
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, timer, cmd](int exitCode, QProcess::ExitStatus exitStatus) {

                qint64 elapsedMs = timer->elapsed();
                delete timer;

                QByteArray rawOutput = process->readAllStandardOutput();
                process->deleteLater();

                // Build a clean diagnostic header & footer
                QByteArray finalContent;
                finalContent.append(QString("--- Command: %1 ---\n").arg(cmd).toUtf8());
                finalContent.append(QString("--- Time elapsed: %1 ms ---\n\n").arg(elapsedMs).toUtf8());

                if (!rawOutput.isEmpty()) {
                    finalContent.append(rawOutput);
                } else {
                    finalContent.append("(Command produced no standard output)\n");
                }

                if (exitStatus != QProcess::NormalExit) {
                    finalContent.append("\n[Error: Process crashed or failed to start normally]\n");
                } else if (exitCode != 0) {
                    finalContent.append(QString("\n[Process exited with non-zero code %1]\n").arg(exitCode).toUtf8());
                }

                // Open a new editor tab for the output
                newFile();
                CodeEditor *outputEditor = currentEditor();
                if (!outputEditor) return;

                // Set syntax as Plain Text
                outputEditor->setLanguage(ProgrammingLanguage::PlainText);

                SyntaxHighlighter *hl = new SyntaxHighlighter(outputEditor->document());
                hl->setLanguage(ProgrammingLanguage::PlainText);
                hl->setTheme(currentTheme);

                outputEditor->setPlainText(QString::fromUtf8(finalContent));
                outputEditor->setReadOnly(true);
                outputEditor->document()->setModified(false);
                updateStatusBarReadWrite(); // Reflect RO status immediately

                // Set descriptive tab title
                int currentIndex = tabWidget->currentIndex();
                tabWidget->setTabText(currentIndex, tr("🖵 %1").arg(cmd.section(' ', 0, 0)));
            });

    // Start the process via shell
#if defined(Q_OS_UNIX)
    process->start("/bin/sh", QStringList() << "-c" << cmd);
#elif defined(Q_OS_WIN)
    process->start("cmd.exe", QStringList() << "/c" << cmd);
#endif
}

void MainWindow::updateStatusBarReadWrite() {
    if (!readWriteLabel) return;

    CodeEditor *editor = currentEditor();
    if (!editor) {
        readWriteLabel->setText("");
        return;
    }

    if (editor->isReadOnly()) {
        readWriteLabel->setText(tr("RO"));
        readWriteLabel->setStyleSheet("QLabel { color: #d9534f; font-weight: bold; }"); // Soft red for Read-Only
    } else {
        readWriteLabel->setText(tr("RW"));
        readWriteLabel->setStyleSheet("QLabel { color: #5cb85c; font-weight: bold; }"); // Soft green for Read-Write
    }
}

void MainWindow::openFileInFollowMode(const QString &filePath) {
    if (activateTabIfAlreadyOpen(filePath)) return;

    if (filePath.isEmpty() || !QFile::exists(filePath)) return;

    // Open a new read-only editor tab for the log file
    newFile();
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    // Load initial content
    QFile *file = new QFile(filePath, editor); // Parented to editor for clean memory management
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Follow Mode"), tr("Could not open file for reading."));
        closeTab(tabWidget->currentIndex());
        return;
    }

    QTextStream in(file);
    editor->setPlainText(in.readAll());
    editor->setReadOnly(true);
    editor->document()->setModified(false);

    // Set syntax as Plain Text
    editor->setLanguage(ProgrammingLanguage::PlainText);

    SyntaxHighlighter *hl = new SyntaxHighlighter(editor->document());
    hl->setLanguage(ProgrammingLanguage::PlainText);
    hl->setTheme(currentTheme);

    updateStatusBarReadWrite();

    // Set descriptive tab title with a indicator
    QFileInfo fileInfo(filePath);
    int currentIndex = tabWidget->currentIndex();
    tabWidget->setTabText(currentIndex, tr("♻ %1").arg(fileInfo.fileName()));
    editor->setProperty("filePath", filePath);
    editor->setProperty("isFollowMode", true);

    // Scroll to the very bottom initially
    editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());

    // Set up a QTimer to poll for changes every 500ms
    QTimer *followTimer = new QTimer(editor);

    // Track the last known file read position & size
    qint64 *lastPos = new qint64(file->pos());

    connect(followTimer, &QTimer::timeout, this, [this, editor, file, lastPos, filePath]() {
        // If the editor tab was closed or destroyed, stop everything
        if (!editor || !file) return;

        // Check if file still exists on disk
        QFileInfo info(filePath);
        if (!info.exists()) return;

        if (file->size() < *lastPos) {
            // File was truncated or rotated! Reset position to 0
            file->seek(0);
            *lastPos = 0;
            editor->clear();
        }

        if (file->size() > *lastPos) {
            file->seek(*lastPos);
            QByteArray newData = file->readAll();
            *lastPos = file->pos();

            // Append new text smoothly
            QTextCursor cursor = editor->textCursor();
            cursor.movePosition(QTextCursor::End);
            cursor.insertText(QString::fromUtf8(newData));
            editor->document()->setModified(false);

            // Auto-scroll to bottom to follow new lines
            editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());

            updateCursorPosition();
        }
    });

    // Start polling every 500 milliseconds
    followTimer->start(500);
}

void MainWindow::setupFavoritesMenu() {
    if (!favoritesMenu) {
        favoritesMenu = menuBar()->addMenu(tr("&Favorites"));
    } else {
        favoritesMenu->clear();
    }

    regularFavMenu = favoritesMenu->addMenu(tr("Regular Files"));
    followedFavMenu = favoritesMenu->addMenu(tr("Followed Files"));
    favoritesMenu->addSeparator();

    QAction *clearFavAction = favoritesMenu->addAction(tr("Clear All Favorites"));
    connect(clearFavAction, &QAction::triggered, this, [this]() {
        QSettings settings(appConstants->getQString("ORGANIZATION_NAME"), appConstants->getQString("APPLICATION_NAME"));
        settings.remove("Favorites/RegularFiles");
        settings.remove("Favorites/FollowedFiles");
        setupFavoritesMenu();
    });

    QSettings settings(appConstants->getQString("ORGANIZATION_NAME"), appConstants->getQString("APPLICATION_NAME"));
    QStringList regularFiles = settings.value("Favorites/RegularFiles").toStringList();
    QStringList followedFiles = settings.value("Favorites/FollowedFiles").toStringList();

    // Regular Files Submenu ---
    regularFavMenu->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(regularFavMenu, &QMenu::customContextMenuRequested, this, [this](const QPoint &pos) {
        QAction *action = regularFavMenu->actionAt(pos);
        if (!action || !action->data().isValid()) return;

        QString path = action->data().toString();
        QMenu contextMenu(this);
        QAction *removeAction = contextMenu.addAction(tr("Remove from Favorites"));

        if (contextMenu.exec(regularFavMenu->mapToGlobal(pos)) == removeAction) {
            removeFromFavorites(path, false);
        }
    });

    if (regularFiles.isEmpty()) {
        QAction *emptyAction = regularFavMenu->addAction(tr("(Empty)"));
        emptyAction->setEnabled(false);
    } else {
        for (const QString &path : regularFiles) {
            QFileInfo info(path);
            QAction *action = regularFavMenu->addAction(info.fileName());
            action->setData(path); // Store full path in action data
            connect(action, &QAction::triggered, this, [this, path]() {
                openFileWithPath(path);
            });
        }
    }

    // --- 2. Followed Files Submenu ---
    followedFavMenu->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(followedFavMenu, &QMenu::customContextMenuRequested, this, [this](const QPoint &pos) {
        QAction *action = followedFavMenu->actionAt(pos);
        if (!action || !action->data().isValid()) return;

        QString path = action->data().toString();
        QMenu contextMenu(this);
        QAction *removeAction = contextMenu.addAction(tr("Remove from Favorites"));

        if (contextMenu.exec(followedFavMenu->mapToGlobal(pos)) == removeAction) {
            removeFromFavorites(path, true);
        }
    });

    if (followedFiles.isEmpty()) {
        QAction *emptyAction = followedFavMenu->addAction(tr("(Empty)"));
        emptyAction->setEnabled(false);
    } else {
        for (const QString &path : followedFiles) {
            QFileInfo info(path);
            QAction *action = followedFavMenu->addAction(info.fileName());
            action->setData(path); // Store full path in action data
            connect(action, &QAction::triggered, this, [this, path]() {
                openFileInFollowMode(path);
            });
        }
    }
}

void MainWindow::addToFavorites(const QString &filePath, bool isFollowMode) {
    if (filePath.isEmpty()) return;

    QSettings settings(appConstants->getQString("ORGANIZATION_NAME"), appConstants->getQString("APPLICATION_NAME"));
    QString key = isFollowMode ? "Favorites/FollowedFiles" : "Favorites/RegularFiles";
    QStringList list = settings.value(key).toStringList();

    if (!list.contains(filePath)) {
        list.append(filePath);
        settings.setValue(key, list);
        setupFavoritesMenu(); // Refresh menu immediately
    }
}

void MainWindow::removeFromFavorites(const QString &filePath, bool isFollowMode) {
    QSettings settings(appConstants->getQString("ORGANIZATION_NAME"), appConstants->getQString("APPLICATION_NAME"));
    QString key = isFollowMode ? "Favorites/FollowedFiles" : "Favorites/RegularFiles";
    QStringList list = settings.value(key).toStringList();

    if (list.removeAll(filePath) > 0) {
        settings.setValue(key, list);
        setupFavoritesMenu(); // Refresh menu immediately
    }
}

bool MainWindow::activateTabIfAlreadyOpen(const QString &filePath) {
    if (filePath.isEmpty()) return false;

    QFileInfo targetInfo(filePath);
    QString canonicalTarget = targetInfo.canonicalFilePath();
    if (canonicalTarget.isEmpty()) canonicalTarget = targetInfo.absoluteFilePath();

    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (!editor) continue;

        QString currentPath = editor->property("filePath").toString();
        if (!currentPath.isEmpty()) {
            QFileInfo currentInfo(currentPath);
            QString canonicalCurrent = currentInfo.canonicalFilePath();
            if (canonicalCurrent.isEmpty()) canonicalCurrent = currentInfo.absoluteFilePath();

            if (canonicalCurrent == canonicalTarget) {
                tabWidget->setCurrentIndex(i); // Bring existing tab to front
                return true;
            }
        }
    }
    return false;
}

void MainWindow::checkForUpdates() {
    qDebug() << "Update Manager called";
    if (!updateManager) {
        updateManager = new QNetworkAccessManager(this);
    }

    QNetworkRequest request(QUrl("https://api.github.com/repos/jplozf/Qute/commits/HEAD"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qute-TextEditor");

    QNetworkReply *reply = updateManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            return; // Fail silently if offline or rate-limited
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        // GitHub's /commits/HEAD endpoint returns a commit object with a "sha" field
        QString remoteFullSha = obj["sha"].toString();
        QString commitUrl = obj["html_url"].toString();

        if (remoteFullSha.isEmpty()) return;

        // Take the short hash (first 7 characters, matching Git's standard short hash)
        QString remoteShortSha = remoteFullSha.left(7);

        // Assumes you have a CMake macro for your local short hash, e.g., GIT_HASH
        // (Make sure your CMake target_compile_definitions includes it!)
        QString localShortSha = QString(GIT_HASH).trimmed();

        if (!localShortSha.isEmpty() && remoteShortSha != localShortSha) {
            qDebug() << "Update found";
            QMessageBox::StandardButton replyBox = QMessageBox::information(
                this,
                tr("Update Available"),
                tr("A newer version is available on GitHub !\n\n"
                   "%1 : Local version\n"
                   "%2 : Remote version\n\n"
                   "Would you like to open the repository in your browser ?")
                    .arg(localShortSha)
                    .arg(remoteShortSha),
                QMessageBox::Yes | QMessageBox::No
                );

            if (replyBox == QMessageBox::Yes) {
                QDesktopServices::openUrl(QUrl(commitUrl));
            }
        } else {
            qDebug() << "No Update available";
        }
    });
}

void MainWindow::goToLine() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;

    int totalLines = editor->document()->blockCount();
    QTextCursor cursor = editor->textCursor();
    int currentLine = cursor.blockNumber() + 1;

    bool ok;
    QString input = QInputDialog::getText(
        this,
        tr("Go to Line"),
        tr("Enter line (e.g., 42, begin, end, +10, -5, 42%):"),
        QLineEdit::Normal,
        QString::number(currentLine),
        &ok
        );

    if (!ok || input.isEmpty()) return;

    input = input.trimmed().toLower();
    int targetLine = currentLine;

    // 1. Keywords: begin / top / end / bottom
    if (input == "begin" || input == "top") {
        targetLine = 1;
    }
    else if (input == "end" || input == "bottom") {
        targetLine = totalLines;
    }
    // 2. Relative jumps: +N or -N
    else if (input.startsWith('+') || input.startsWith('-')) {
        bool validOffset = false;
        int offset = input.toInt(&validOffset);
        if (validOffset) {
            targetLine = currentLine + offset;
        }
    }
    // 3. Percentages: 42% or %42
    else if (input.endsWith('%') || input.startsWith('%')) {
        QString numPart = input;
        if (numPart.endsWith('%')) numPart.chop(1);
        if (numPart.startsWith('%')) numPart = numPart.mid(1);

        bool validPercent = false;
        double percent = numPart.trimmed().toDouble(&validPercent);
        if (validPercent) {
            percent = qBound(0.0, percent, 100.0);
            targetLine = qRound((percent / 100.0) * totalLines);
            if (targetLine < 1) targetLine = 1;
        }
    }
    // 4. Absolute line number
    else {
        bool validNum = false;
        int num = input.toInt(&validNum);
        if (validNum) {
            targetLine = num;
        }
    }

    // Clamp the target line between 1 and totalLines to prevent out-of-bounds crashes
    targetLine = qBound(1, targetLine, totalLines);

    // Move cursor and center view
    QTextBlock block = editor->document()->findBlockByNumber(targetLine - 1);
    if (block.isValid()) {
        cursor.setPosition(block.position());
        editor->setTextCursor(cursor);
        editor->centerCursor();
    }
}

QString MainWindow::getGitWorkingDir() {
    CodeEditor *editor = currentEditor();
    if (editor) {
        // Get the full file path from your editor or window tracker
        QString filePath = editor->property("filePath").toString(); // or your specific file path getter

        if (!filePath.isEmpty()) {
            QFileInfo fileInfo(filePath);
            // .absolutePath() correctly returns just the directory (e.g. /home/jpl/Projets/Go/jplozf/bled)
            return fileInfo.absolutePath();
        }
    }
    return QDir::currentPath();
}

void MainWindow::runGitCommand(const QStringList &args, const QString &successMessage, bool captureOutput) {
    QString workDir = getGitWorkingDir();
    // Open a new editor tab for the output
    newFile();
    CodeEditor *outputEditor = currentEditor();
    if (!outputEditor) return;

    // Set syntax as Plain Text
    outputEditor->setLanguage(ProgrammingLanguage::PlainText);

    SyntaxHighlighter *hl = new SyntaxHighlighter(outputEditor->document());
    hl->setLanguage(ProgrammingLanguage::PlainText);
    hl->setTheme(currentTheme);

    outputEditor->setReadOnly(true);
    outputEditor->document()->setModified(false);
    updateStatusBarReadWrite(); // Reflect RO status immediately

    // Set descriptive tab title
    int currentIndex = tabWidget->currentIndex();
    tabWidget->setTabText(currentIndex, tr("🖵 git %1").arg(args.join(" ")));


    if (outputEditor) {
        outputEditor->appendPlainText(QString("[Git] $ git %1\n").arg(args.join(" ")));
    }

    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(workDir);

    connect(process, &QProcess::readyReadStandardOutput, this, [this, outputEditor, process]() {
        QString out = QString::fromUtf8(process->readAllStandardOutput());
        if (outputEditor) outputEditor->appendPlainText(out);
    });

    connect(process, &QProcess::readyReadStandardError, this, [this, outputEditor, process]() {
        QString err = QString::fromUtf8(process->readAllStandardError());
        if (outputEditor) outputEditor->appendPlainText(err);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outputEditor, successMessage, process](int exitCode, QProcess::ExitStatus) {
                if (exitCode == 0) {
                    if (!successMessage.isEmpty() && outputEditor) {
                        outputEditor->appendPlainText(QString("[Git Success] %1\n").arg(successMessage));
                    }
                } else {
                    if (outputEditor) {
                        outputEditor->appendPlainText(QString("[Git Error] Process exited with code %1\n").arg(exitCode));
                    }
                }
                if (outputEditor && outputEditor->document()) {
                    outputEditor->document()->setModified(false);
                }
                process->deleteLater();
            });

    // Check working Directory
    if (!QDir(workDir).exists()) {
        if (outputEditor) {
            outputEditor->appendPlainText(tr("[Error] Working directory does not exist: %1\n").arg(workDir));
        }
        process->deleteLater();
        return;
    }

    // Find absolute path to git executable if not in inherited PATH
    QString gitPath = QStandardPaths::findExecutable("git");
    if (gitPath.isEmpty()) {
        // Common fallback paths on Linux
        if (QFile::exists("/usr/bin/git")) gitPath = "/usr/bin/git";
        else if (QFile::exists("/usr/local/bin/git")) gitPath = "/usr/local/bin/git";
        else gitPath = "git"; // Last resort
    }

    process->start(gitPath, args);
    if (!process->waitForStarted(3000)) {
        if (outputEditor) {
            outputEditor->appendPlainText(tr("[Error] Failed to start git process.\nPath: %1\nWorking Dir: %2\nReason: %3\n")
                                              .arg(gitPath, workDir, process->errorString()));
        }
        process->deleteLater();
        return;
    }
}

void MainWindow::setupGitMenu() {
    if (!gitMenu) {
        gitMenu = menuBar()->addMenu(tr("&Git"));
    } else {
        gitMenu->clear();
    }

    struct GitActionItem {
        QString label;
        void (MainWindow::*handler)();
    };

    QList<GitActionItem> entries = {
        {tr("Status"), &MainWindow::doGitStatus},
        {tr("Log"), &MainWindow::doGitLog},
        {tr("Add all (.)"), &MainWindow::doGitAddAll},
        {tr("Commit"), &MainWindow::doGitCommit},
        {tr("Push"), &MainWindow::doGitPush},
        {tr("Commit && Push"), &MainWindow::doGitCommitPush},
        {tr("Fetch"), &MainWindow::doGitFetch},
        {tr("Pull (Fetch && Merge)"), &MainWindow::doGitPull},
        {tr("Initialize"), &MainWindow::doGitInit},
        {tr("Initialize && Push"), &MainWindow::doGitBang},
        {tr("Clone"), &MainWindow::doGitClone}
    };

    for (const auto &entry : entries) {
        QAction *action = gitMenu->addAction(entry.label);
        connect(action, &QAction::triggered, this, entry.handler);
    }
}

void MainWindow::doGitStatus() { runGitCommand({"status"}, "", true); }
void MainWindow::doGitLog() { runGitCommand({"log", "-n", "30", "--oneline"}, "", true); }
void MainWindow::doGitAddAll() { runGitCommand({"add", "."}, tr("Staged all changes (.)")); }

void MainWindow::doGitCommit() {
    bool ok;
    QString msg = QInputDialog::getText(this, tr("Git Commit"), tr("Commit message:"), QLineEdit::Normal, "", &ok);
    if (ok && !msg.isEmpty()) {
        runGitCommand({"commit", "-m", msg}, tr("Changes committed successfully."));
    }
}

void MainWindow::doGitPush() { runGitCommand({"push"}, tr("Pushed to remote successfully.")); }

void MainWindow::doGitCommitPush() {
    bool ok;
    QString msg = QInputDialog::getText(this, tr("Git Commit & Push"), tr("Commit message:"), QLineEdit::Normal, "", &ok);
    if (ok && !msg.isEmpty()) {
        QString workDir = getGitWorkingDir();
        QProcess proc;
        proc.setWorkingDirectory(workDir);
        proc.start("git", {"add", "."});
        proc.waitForFinished();
        proc.start("git", {"commit", "-m", msg});
        proc.waitForFinished();
        proc.start("git", {"push"});
        proc.waitForFinished();
        QMessageBox::information(this, tr("Git"), tr("Commit and Push sequence completed."));
    }
}

void MainWindow::doGitFetch() { runGitCommand({"fetch"}, tr("Fetched latest updates from remote.")); }
void MainWindow::doGitPull() { runGitCommand({"pull"}, tr("Pulled and merged remote changes.")); }
void MainWindow::doGitInit() { runGitCommand({"init"}, tr("Initialized empty Git repository.")); }

void MainWindow::doGitBang() {
    bool ok;
    QString remoteUrl = QInputDialog::getText(this, tr("Initialize & Push"), tr("Remote repository URL:"), QLineEdit::Normal, "", &ok);
    if (ok && !remoteUrl.isEmpty()) {
        QString workDir = getGitWorkingDir();
        QProcess proc;
        proc.setWorkingDirectory(workDir);
        proc.start("git", {"init"});
        proc.waitForFinished();
        proc.start("git", {"remote", "add", "origin", remoteUrl});
        proc.waitForFinished();
        proc.start("git", {"add", "."});
        proc.waitForFinished();
        proc.start("git", {"commit", "-m", "Initial commit"});
        proc.waitForFinished();
        proc.start("git", {"push", "-u", "origin", "main"});
        proc.waitForFinished();
        QMessageBox::information(this, tr("Git"), tr("Repository initialized and pushed successfully."));
    }
}

void MainWindow::doGitClone() {
    bool ok;
    QString url = QInputDialog::getText(this, tr("Git Clone"), tr("Repository URL to clone:"), QLineEdit::Normal, "", &ok);
    if (ok && !url.isEmpty()) {
        runGitCommand({"clone", url}, tr("Repository cloned successfully."));
    }
}

void MainWindow::setupSystemTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    trayIcon = new QSystemTrayIcon(this);
    // Use application window icon or fallback system icon
    trayIcon->setIcon(windowIcon().isNull() ? style()->standardIcon(QStyle::SP_ComputerIcon) : windowIcon());

    trayMenu = new QMenu(this);
    updateTrayMenu();

    trayIcon->setContextMenu(trayMenu);

    // Left-click or double-click restores/hides the main window
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if (isVisible()) {
                hide();
            } else {
                show();
                raise();
                activateWindow();
            }
        }
    });

    trayIcon->show();
}

void MainWindow::updateTrayMenu() {
    if (!trayMenu) return;
    trayMenu->clear();

    // Dynamic Open Files Submenu / List
    int count = tabWidget->count();
    if (count > 0) {
        QMenu *filesMenu = trayMenu->addMenu(tr("Open Files"));
        for (int i = 0; i < count; ++i) {
            QString title = tabWidget->tabText(i);
            QAction *fileAction = filesMenu->addAction(title);
            connect(fileAction, &QAction::triggered, this, [this, i]() {
                tabWidget->setCurrentIndex(i);
                show();
                raise();
                activateWindow();
            });
        }
        trayMenu->addSeparator();
    }

    // New File Option
    QAction *newAction = trayMenu->addAction(tr("New File"));
    connect(newAction, &QAction::triggered, this, [this]() {
        newFile();
        show();
        raise();
        activateWindow();
    });

    trayMenu->addSeparator();

    // Quit Option (Forces full exit bypassing tray check)
    QAction *quitAction = trayMenu->addAction(tr("Quit Qute"));
    connect(quitAction, &QAction::triggered, this, [this]() {
        // Disconnect or set flag so closeEvent allows termination
        m_minimizeToTray = false;
        m_forceQuit = true;
        close();
    });
}

void MainWindow::toggleFullScreen() {
    if (isFullScreen()) {
        showNormal(); // Restores previous window geometry/state
    } else {
        showFullScreen(); // Expands to full screen
    }
}

void MainWindow::updateFunctionOutline() {
    m_functionComboBox->blockSignals(true);
    m_functionComboBox->clear();
    m_functionComboBox->addItem(tr("-- Functions --"), -1);

    // Structure to hold function info for sorting
    struct FunctionItem {
        QString name;
        int blockNumber;
    };

    QList<FunctionItem> foundFunctions;

    CodeEditor *editor = currentEditor();
    if (!editor) {
        m_functionComboBox->blockSignals(false);
        return;
    }

    ProgrammingLanguage lang = editor->language();
    QTextDocument *doc = editor->document();

    QRegularExpression cppRegex(R"(^\s*(?:[\w\s\*&:<>,~]+?\s+)?([a-zA-Z_]\w*(?:::\w+)?)\s*\([^;]*\)\s*(?:const)?\s*(?:noexcept)?\s*(?:\{|$))");
    QRegularExpression goRegex(R"(^func\s+(?:\([^)]+\)\s+)?([a-zA-Z_]\w*)\s*\()");
    QRegularExpression pyRegex(R"(^\s*(?:async\s+)?def\s+([a-zA-Z_]\w*)\s*\()");
    QRegularExpression jsRegex(R"(^\s*(?:async\s+)?(?:function\s+([a-zA-Z_]\w*)|([a-zA-Z_]\w*)\s*[:=]\s*(?:async\s+)?(?:\([^)]*\)|[a-zA-Z_]\w*)\s*=>|([a-zA-Z_]\w*)\s*\([^)]*\)\s*\{))");

    // Iterate through document blocks (physical lines) instead of split strings
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        int blockNumber = block.blockNumber(); // 0-based physical line index
        QString line = block.text().trimmed();
        QString funcName;

        if (line.startsWith("//") || line.startsWith("/*") || line.startsWith("*") || line.startsWith("#")) {
            continue;
        }

        if (lang == ProgrammingLanguage::Python) {
            QRegularExpressionMatch match = pyRegex.match(line);
            if (match.hasMatch()) funcName = match.captured(1);
        }
        else if (lang == ProgrammingLanguage::JavaScript) {
            QRegularExpressionMatch match = jsRegex.match(line);
            if (match.hasMatch()) {
                for (int g = 1; g <= match.lastCapturedIndex(); ++g) {
                    if (!match.captured(g).isEmpty()) {
                        funcName = match.captured(g);
                        break;
                    }
                }
            }
        }
        else if (lang == ProgrammingLanguage::Go) {
            QRegularExpressionMatch match = goRegex.match(line);
            if (match.hasMatch()) funcName = match.captured(1);
        }
        else { // C / C++
            if (line.contains("(") && line.contains(")") && !line.contains(";")) {
                QRegularExpressionMatch match = cppRegex.match(line);
                if (match.hasMatch()) {
                    funcName = match.captured(1);
                    if (funcName == "if" || funcName == "while" || funcName == "for" ||
                        funcName == "switch" || funcName == "catch" || funcName == "return" ||
                        funcName == "sizeof" || funcName == "decltype") {
                        funcName.clear();
                    }
                }
            }
        }

        if (!funcName.isEmpty()) {
            foundFunctions.append({funcName, blockNumber});
        }
    }

    // Sort alphabetically by function name (case-insensitive)
    std::sort(foundFunctions.begin(), foundFunctions.end(), [](const FunctionItem &a, const FunctionItem &b) {
        return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
    });

    // Populate the combobox with the sorted items
    for (const auto &func : foundFunctions) {
        m_functionComboBox->addItem(func.name, func.blockNumber);
    }

    m_functionComboBox->blockSignals(false);
}

void MainWindow::jumpToSelectedFunction(int index) {
    if (index <= 0) return; // Skip the "-- Functions --" header

    int blockNumber = m_functionComboBox->itemData(index).toInt();
    if (blockNumber < 0) return;

    CodeEditor *editor = currentEditor();
    if (!editor) return;

    // Retrieve the exact document block by its absolute index
    QTextDocument *doc = editor->document();
    QTextBlock targetBlock = doc->findBlockByNumber(blockNumber);

    if (targetBlock.isValid()) {
        // Create a cursor at the very beginning of that block
        QTextCursor cursor(targetBlock);

        editor->setTextCursor(cursor);
        editor->centerCursor(); // Center the line in the viewport
        editor->setFocus();
    }
}

void MainWindow::closeAllOutputTabs() {
    // Iterate backwards to safely remove tabs without invalidating indices
    for (int i = tabWidget->count() - 1; i >= 0; --i) {
        QWidget *widget = tabWidget->widget(i);
        // Check if the tab is tagged as an output tab
        if (widget && tabWidget->tabText(i).startsWith("🖵 ")) {
            // Remove the tab from the widget container
            tabWidget->removeTab(i);

            // Schedule the widget for safe deletion
            widget->deleteLater();
        }
    }
}

void MainWindow::setupTemplatesMenu(QMenu *parent) {
    // Create the "New from Template" submenu
    QMenu *templateMenu = parent->addMenu(tr("New &from Template"));

    // Determine path: ~/.config/YourOrganization/Qute/templates
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + "/" + appConstants->getQString("ORGANIZATION_NAME")
                        + "/" + appConstants->getQString("APPLICATION_NAME");
    QDir().mkpath(configDir); // Ensure directory exists
    QString templatesDirPath = configDir + "/templates";

    QDir templatesDir(templatesDirPath);
    if (!templatesDir.exists()) {
        templatesDir.mkpath(".");
        QDir(templatesDirPath + "/C").mkpath(".");
        QFile::copy(":/templates/default_c.c", templatesDirPath + "/C/C Source Code.c");
    }

    bool foundAnyTemplates = false;

    // SCAN ROOT-LEVEL TEMPLATE FILES (directly in templates/)
    QStringList rootFiles = templatesDir.entryList(QDir::Files);
    for (const QString &fileName : rootFiles) {
        QString filePath = templatesDir.absoluteFilePath(fileName);
        QFileInfo fileInfo(fileName);
        QString displayTitle = fileInfo.fileName();

        QAction *templateAction = templateMenu->addAction(displayTitle);
        connect(templateAction, &QAction::triggered, this, [this, filePath]() {
            loadTemplateFile(filePath);
        });
        foundAnyTemplates = true;
    }

    // Add a separator if we had root files and are about to add submenus
    if (!rootFiles.isEmpty()) {
        templateMenu->addSeparator();
    }

    // SCAN LANGUAGE SUBDIRECTORIES (e.g., C, C++, Go, Python)
    QStringList languageFolders = templatesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &langName : languageFolders) {
        QDir langDir(templatesDir.absoluteFilePath(langName));
        QStringList templateFiles = langDir.entryList(QDir::Files);

        if (templateFiles.isEmpty()) continue; // Skip empty folders

        QMenu *langMenu = templateMenu->addMenu(langName);

        for (const QString &fileName : templateFiles) {
            QString filePath = langDir.absoluteFilePath(fileName);
            QFileInfo fileInfo(fileName);
            QString displayTitle = fileInfo.fileName();

            QAction *templateAction = langMenu->addAction(displayTitle);
            connect(templateAction, &QAction::triggered, this, [this, filePath]() {
                loadTemplateFile(filePath);
            });
        }
        foundAnyTemplates = true;
    }

    // If completely empty, show a disabled placeholder
    if (!foundAnyTemplates) {
        QAction *emptyAction = templateMenu->addAction(tr("No templates found"));
        emptyAction->setEnabled(false);
    }

    templateMenu->addSeparator();

    QAction *openFolderAction = templateMenu->addAction(tr("Open Templates Folder..."));
    connect(openFolderAction, &QAction::triggered, this, [templatesDirPath]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(templatesDirPath));
    });
}

void MainWindow::loadTemplateFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Template Error"), tr("Could not read template file:\n%1").arg(filePath));
        return;
    }

    QString templateContent = QString::fromUtf8(file.readAll());
    file.close();

    // Create a new tab with the template content
    CodeEditor *editor = new CodeEditor(this);
    editor->setFont(currentEditorFont);
    editor->setTheme(currentTheme);

    SyntaxHighlighter *hl = new SyntaxHighlighter(editor->document());
    hl->setTheme(currentTheme);

    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::documentModified);
    connect(editor, &QPlainTextEdit::textChanged, this, [this]() {
        m_outlineUpdateTimer->start(); // Restarts the countdown on every keystroke
    });

    editor->setPlainText(templateContent);
    editor->document()->setModified(true);
    QString title = QFileInfo(filePath).fileName();
    int index = tabWidget->addTab(editor, title + " *");
    tabWidget->setCurrentIndex(index);
    tabWidget->setCurrentWidget(editor);
    updateCursorPosition();
    updateTrayMenu();
}