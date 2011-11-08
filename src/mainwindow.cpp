#include <QtGui>

#include "mainwindow.h"
#include "defines.h"
#include "logger.h"
#include "qarkdownapplication.h"

/*
TODO:
- Support relative font sizes in PMH stylesheets (e.g. +3pt or -2pt)
- Highlight whole blockquotes in PMH

- Make sure that the quit save confirmation works correctly everywhere
- Start using QSingleApplication ??

- Fix the tab/shift-tab indentation to work in a more "standard" manner
- Windows: Fix/workaround for non-working text color alpha
- OS X: Catch the maximize/zoom action (window button + menu item) and set custom "zoomed" size
- OS X: Make main window title bar show the file icon + provide the file path dropdown

- Clear status bar when file opened successfully
- Use tr() for all UI strings

- Multiple files open; file switcher dock widget
- Use QTextOption::ShowTabsAndSpaces
- Use Sparkle on OS X
*/

#define kUntitledFileUIName "Untitled"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    settings = new QSettings("org.hasseg", "QarkDown");
    compiler = new MarkdownCompiler(settings);

    // PreferencesDialog depends on the HGUpdateCheck settings being
    // set, so we have to set that up first:
    HGUpdateCheck::setUpdateCheckSettings(settings);
    updateCheck = new HGUpdateCheck(((QarkdownApplication *)qApp)->websiteURL(), this);
    updateCheck->handleAppStartup();

    preferencesDialog = new PreferencesDialog(settings, compiler);

    recentFilesMenuActions = new QList<QAction *>();

    setupFileMenu();
    setupEditor();
    performStartupTasks();
    setCentralWidget(editor);

    statusBar()->show();
}

MainWindow::~MainWindow()
{
    delete updateCheck;
    delete settings;
    delete preferencesDialog;
    delete compiler;
    delete recentFilesMenuActions;
}

void MainWindow::show()
{
    QSize defaultSize(500, 700);
    resize(defaultSize);

    bool rememberWindow = settings->value(SETTING_REMEMBER_WINDOW,
                                          QVariant(DEF_REMEMBER_WINDOW)).toBool();
    if (rememberWindow) {
        restoreGeometry(settings->value(SETTING_WINDOW_GEOMETRY).toByteArray());
        restoreState(settings->value(SETTING_WINDOW_STATE).toByteArray());
    }

    QMainWindow::show();
}

void MainWindow::newFile()
{
    editor->clear();
    openFilePath = QString();
    revertToSavedMenuAction->setEnabled(false);
    lastCompileTargetPath = QString();
    recompileAction->setEnabled(false);
    setDirty(false);
    updateRecentFilesMenu();
}

QString MainWindow::getMarkdownFilesFilter()
{
    QStringList extensions = settings->value(SETTING_EXTENSIONS, DEF_EXTENSIONS)
                             .toString().split(' ', QString::SkipEmptyParts);
    if (extensions.count() == 0)
        return "All Files (*.*)";

    QString filesFilter = "Markdown Files (";
    foreach (QString ext, extensions)
    {
        QString cleanExt = ext.trimmed();
        if (cleanExt.startsWith("."))
            cleanExt = cleanExt.remove(0,1);
        filesFilter += "*." + cleanExt + " ";
    }
    filesFilter.chop(1); // remove last space
    filesFilter += ")";
    return filesFilter;
}

void MainWindow::openFile(const QString &path)
{
    QString fileName = path;

    if (fileName.isNull())
        fileName = QFileDialog::getOpenFileName(this,
            tr("Open File"), "", getMarkdownFilesFilter());

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        statusBar()->showMessage("Cannot open: " + fileName
                                 + " (reason: " + file.errorString() + ")");
        return;
    }

    QTextStream inStream(&file);
    inStream.setCodec("UTF-8");
    editor->setPlainText(inStream.readAll());
    file.close();

    openFilePath = fileName;
    revertToSavedMenuAction->setEnabled(true);
    lastCompileTargetPath = QString();
    recompileAction->setEnabled(false);

    setDirty(false);
    bool rememberLastFile = settings->value(SETTING_REMEMBER_LAST_FILE,
                                            QVariant(DEF_REMEMBER_LAST_FILE)).toBool();
    if (rememberLastFile) {
        settings->setValue(SETTING_LAST_FILE, QVariant(openFilePath));
        settings->sync();
    }
    addToRecentFiles(openFilePath);
    updateRecentFilesMenu();
}

void MainWindow::saveFile()
{
    bool savingNewFile = (openFilePath.isNull());

    QString saveFilePath(openFilePath);
    if (saveFilePath.isNull())
        saveFilePath = QFileDialog::getSaveFileName(this,
            tr("Save File"), "", getMarkdownFilesFilter());

    if (saveFilePath.isEmpty())
        return;

    QFile file(saveFilePath);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;
    QTextStream outStream(&file);
    outStream.setCodec("UTF-8");
    outStream << editor->toPlainText();
    file.close();

    if (savingNewFile)
    {
        bool rememberLastFile = settings->value(SETTING_REMEMBER_LAST_FILE,
                                                QVariant(DEF_REMEMBER_LAST_FILE)).toBool();
        if (rememberLastFile) {
            settings->setValue(SETTING_LAST_FILE, QVariant(saveFilePath));
            settings->sync();
        }
        addToRecentFiles(saveFilePath);
        updateRecentFilesMenu();
    }

    openFilePath = saveFilePath;
    revertToSavedMenuAction->setEnabled(true);
    setWindowTitle(QFileInfo(openFilePath).fileName());
    setDirty(false);
}

void MainWindow::revertToSaved()
{
    if (openFilePath.isNull())
        return;
    openFile(openFilePath);
}

void MainWindow::addToRecentFiles(QString filePath)
{
    QStringList recentFiles = settings->value(SETTING_RECENT_FILES).toStringList();

    int index = recentFiles.indexOf(filePath);
    if (-1 < index)
        recentFiles.removeAt(index);
    recentFiles.insert(0, filePath);

    int maxNumRecentFiles = settings->value(SETTING_NUM_RECENT_FILES, DEF_NUM_RECENT_FILES).toInt();
    while (maxNumRecentFiles < recentFiles.count())
        recentFiles.removeLast();

    settings->setValue(SETTING_RECENT_FILES, recentFiles);
    settings->sync();
}

void MainWindow::persistFontInfo()
{
    settings->setValue(SETTING_FONT, QVariant(editor->font().toString()));
    settings->sync();
}
void MainWindow::applyPersistedFontInfo()
{
    // font
    QFont font;
    if (settings->contains(SETTING_FONT))
        font.fromString(settings->value(SETTING_FONT).toString());
    else {
        font.setFamily(DEF_FONT_FAMILY);
        font.setPointSize(DEF_FONT_SIZE);
        font.setFixedPitch(true);
    }
    editor->setFont(font);

    // tab stop width (dependent on font)
    int tabWidthInChars = settings->value(SETTING_TAB_WIDTH,
                                          QVariant(DEF_TAB_WIDTH)).toInt();
    QFontMetrics fontMetrics(font);
    editor->setTabStopWidth(fontMetrics.charWidth("m", 0) * tabWidthInChars);
}

void MainWindow::selectTextToSearchFor()
{
    bool ok;
    QString str = QInputDialog::getText(this, tr("Find Text"),
                                        tr("Enter text to find:"),
                                        QLineEdit::Normal, searchString, &ok);
    if (!ok || str.isEmpty())
        return;
    searchString = str;
    findNextMenuAction->setEnabled(true);
    findPreviousMenuAction->setEnabled(true);
    findNextSearchMatch();
}

void MainWindow::findNextSearchMatch()
{
    if (searchString.isEmpty())
        return;
    editor->find(searchString);
}

void MainWindow::findPreviousSearchMatch()
{
    if (searchString.isEmpty())
        return;
    editor->find(searchString, QTextDocument::FindBackward);
}

void MainWindow::increaseFontSize()
{
    QFont font(editor->font());
    font.setPointSize(editor->font().pointSize() + 1);
    editor->setFont(font);
    persistFontInfo();
}

void MainWindow::decreaseFontSize()
{
    QFont font(editor->font());
    font.setPointSize(editor->font().pointSize() - 1);
    editor->setFont(font);
    persistFontInfo();
}

void MainWindow::about()
{
    QString title = tr("About %1").arg(QCoreApplication::applicationName());
    QString msg =
            tr("Version %1"
               "\n\n"
               "Copyright © %2 %3"
               "\n\n"
               "%4")
            .arg(QCoreApplication::applicationVersion())
            .arg(((QarkdownApplication *)qApp)->copyrightYear())
            .arg(QCoreApplication::organizationName())
            .arg(((QarkdownApplication *)qApp)->websiteURL());
    QMessageBox aboutBox;
    aboutBox.setIconPixmap(QPixmap(":/smallAppIcon.png"));
    aboutBox.setText(title);
    aboutBox.setInformativeText(msg);
    aboutBox.exec();
}

void MainWindow::checkForUpdates()
{
    updateCheck->checkForUpdatesNow();
}

void MainWindow::applyHighlighterPreferences()
{
    double highlightInterval = settings->value(SETTING_HIGHLIGHT_INTERVAL,
                                               QVariant(DEF_HIGHLIGHT_INTERVAL)).toDouble();
    highlighter->setWaitInterval(highlightInterval);

    bool clickableLinks = settings->value(SETTING_CLICKABLE_LINKS,
                                          QVariant(DEF_CLICKABLE_LINKS)).toBool();
    highlighter->setMakeLinksClickable(clickableLinks);

    // Apply style
    connect(highlighter, SIGNAL(styleParsingErrors(QList<QPair<int, QString> >*)),
            this, SLOT(reportStyleParsingErrors(QList<QPair<int, QString> >*)));
    QString styleFilePath = settings->value(SETTING_STYLE,
                                            QVariant(DEF_STYLE)).toString();
    if (!QFile::exists(styleFilePath))
    {
        QMessageBox::warning(this, "Error loading style",
                             "Cannot load style file:\n'"+styleFilePath+"'"
                             "\n\n"
                             "Falling back to default style.");
        styleFilePath = DEF_STYLE;
        settings->setValue(SETTING_STYLE, DEF_STYLE);
        settings->sync();
    }
    highlighter->getStylesFromStylesheet(styleFilePath, editor);
    editor->setCurrentLineHighlightColor(highlighter->currentLineHighlightColor);
    editor->setLineNumberAreaColor(editor->palette().base().color().darker(140));
}

void MainWindow::applyEditorPreferences()
{
    // Indentation
    bool indentWithTabs = settings->value(SETTING_INDENT_WITH_TABS,
                                          QVariant(DEF_INDENT_WITH_TABS)).toBool();
    int tabWidthInChars = settings->value(SETTING_TAB_WIDTH,
                                          QVariant(DEF_TAB_WIDTH)).toInt();
    editor->setSpacesIndentWidthHint(tabWidthInChars);
    if (indentWithTabs)
        editor->setIndentString("\t");
    else
    {
        QString indentStr = " ";
        for (int i = 1; i < tabWidthInChars; i++)
            indentStr += " ";
        editor->setIndentString(indentStr);
    }

    // Current line highlighting
    bool highlightCurrentLine = settings->value(SETTING_HIGHLIGHT_CURRENT_LINE,
                                                QVariant(DEF_HIGHLIGHT_CURRENT_LINE)).toBool();
    editor->setHighlightCurrentLine(highlightCurrentLine);

    // Formatting
    bool emphWithUnderscores = settings->value(SETTING_FORMAT_EMPH_WITH_UNDERSCORES,
                                               QVariant(DEF_FORMAT_EMPH_WITH_UNDERSCORES)).toBool();
    editor->setFormatEmphasisWithUnderscores(emphWithUnderscores);
    bool strongWithUnderscores = settings->value(SETTING_FORMAT_STRONG_WITH_UNDERSCORES,
                                                 QVariant(DEF_FORMAT_STRONG_WITH_UNDERSCORES)).toBool();
    editor->setFormatStrongWithUnderscores(strongWithUnderscores);
}

void MainWindow::showPreferences()
{
    preferencesDialog->setModal(true);
    preferencesDialog->show();
}

void MainWindow::preferencesUpdated()
{
    applyPersistedFontInfo();
    applyHighlighterPreferences();
    applyEditorPreferences();
    highlighter->highlightNow();
}

bool MainWindow::isDirty()
{
    return editor->document()->isModified();
}

void MainWindow::setDirty(bool value)
{
    editor->document()->setModified(value);
    QString dirtyFlagStr = (value ? " **" : "");
    if (!openFilePath.isNull())
        setWindowTitle(QFileInfo(openFilePath).fileName()+dirtyFlagStr);
    else
        setWindowTitle(kUntitledFileUIName+dirtyFlagStr);
}


void MainWindow::setupEditor()
{
    editor = new QarkdownTextEdit;
    editor->setAnchorClickKeyboardModifiers(Qt::ControlModifier);
    highlighter = new HGMarkdownHighlighter(editor->document());

    applyPersistedFontInfo();
    applyHighlighterPreferences();
    applyEditorPreferences();
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    openFile(action->data().toString());
}

QString getTempHTMLFilePathForMarkdownFilePath(QString markdownFilePath)
{
    QString tempDirPath = QDir::tempPath();
    QString tempFileExtension = ".html";

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(markdownFilePath.toUtf8());
    QString tempFileNameBase = "qarkdown-" + hash.result().toHex();
    QString tempFilePath = tempDirPath + QDir::separator()
                           + tempFileNameBase + tempFileExtension;
    if (QFile::exists(tempFilePath))
        QFile::remove(tempFilePath);
    return tempFilePath;
}

void MainWindow::compileToTempHTML()
{
    if (openFilePath.isNull())
        return;
    QString tempFilePath = getTempHTMLFilePathForMarkdownFilePath(openFilePath);

    if (compileToHTMLFile(tempFilePath))
        QDesktopServices::openUrl(QUrl("file:///" + tempFilePath));
}
void MainWindow::compileToHTMLAs()
{
    QString saveFilePath = QFileDialog::getSaveFileName(this,
        tr("Save HTML Output"), "");

    if (saveFilePath.isNull())
        return;

    bool openAfterCompiling = settings->value(SETTING_OPEN_TARGET_AFTER_COMPILING,
                                              DEF_OPEN_TARGET_AFTER_COMPILING).toBool();
    if (compileToHTMLFile(saveFilePath) && openAfterCompiling)
        QDesktopServices::openUrl(QUrl("file:///" + saveFilePath));
}
void MainWindow::recompileToHTML()
{
    if (lastCompileTargetPath.isNull())
        return;
    compileToHTMLFile(lastCompileTargetPath);
}

bool MainWindow::compileToHTMLFile(QString targetPath)
{
    QString compilerPath = settings->value(SETTING_COMPILER,
                                           QVariant(DEF_COMPILER)).toString();
    if (!QFile::exists(compilerPath)) {
        QMessageBox::warning(this, "Cannot compile",
                             "The Markdown to HTML compiler cannot be found at:\n"
                             "'" + compilerPath + "'");
        return false;
    }
    statusBar()->showMessage("Compiling to " + targetPath + "...");
    bool success = compiler->compileToHTMLFile(compilerPath, editor->toPlainText(),
                                               targetPath);
    recompileAction->setEnabled(true);
    if (success)
    {
        lastCompileTargetPath = targetPath;
        statusBar()->showMessage("Compiled successfully to: " + targetPath, 3000);
    }
    else
    {
        QString cleanCompilerPath = compiler->getUserReadableCompilerName(compilerPath);
        QString message = "Compiling failed with compiler:\n" + cleanCompilerPath;
        if (!compiler->errorString().isNull())
            message += "\n\n" + compiler->errorString();
        QMessageBox::warning(this, "Compiling Failed", message);
        statusBar()->showMessage("Compiling failed to: " + targetPath, 3000);
    }
    return success;
}


void MainWindow::formatSelectionEmphasized()
{
    editor->toggleFormattingForCurrentSelection(QarkdownTextEdit::Emphasized);
}
void MainWindow::formatSelectionStrong()
{
    editor->toggleFormattingForCurrentSelection(QarkdownTextEdit::Strong);
}
void MainWindow::formatSelectionCode()
{
    editor->toggleFormattingForCurrentSelection(QarkdownTextEdit::Code);
}



void MainWindow::updateRecentFilesMenu()
{
    for (int i = 0; i < recentFilesMenuActions->count(); i++)
    {
        QAction *action = recentFilesMenuActions->at(i);
        disconnect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        delete action;
    }
    recentFilesMenuActions->clear();

    recentFilesMenu->clear();
    QStringList recentFiles = settings->value(SETTING_RECENT_FILES).toStringList();
    foreach (QString recentFilePath, recentFiles)
    {
        if (!openFilePath.isEmpty() && openFilePath == recentFilePath)
            continue;
        QAction *action = new QAction(this);
        action->setText(QFileInfo(recentFilePath).fileName());
        action->setToolTip(recentFilePath);
        action->setStatusTip(recentFilePath);
        action->setData(recentFilePath);
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        recentFilesMenuActions->append(action);
        recentFilesMenu->addAction(action);
    }
}

void MainWindow::setupFileMenu()
{
    QMenu *fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);
    fileMenu->addAction(tr("&New"), this, SLOT(newFile()),
                        QKeySequence::New);
    fileMenu->addAction(tr("&Open..."), this, SLOT(openFile()),
                        QKeySequence::Open);
    recentFilesMenu = new QMenu(tr("Open Recent..."), this);
    fileMenu->addMenu(recentFilesMenu);
    fileMenu->addAction(tr("&Save"), this, SLOT(saveFile()),
                        QKeySequence::Save);
    revertToSavedMenuAction = fileMenu->addAction(tr("&Revert to Saved"), this,
                                                  SLOT(revertToSaved()));
    revertToSavedMenuAction->setEnabled(false);
    fileMenu->addAction(tr("E&xit"), this, SLOT(quitActionHandler()),
                        QKeySequence::Quit);

    QMenu *editMenu = new QMenu(tr("&Edit"), this);
    menuBar()->addMenu(editMenu);
    editMenu->addAction(tr("&Find..."), this, SLOT(selectTextToSearchFor()),
                        QKeySequence::Find);
    findNextMenuAction = editMenu->addAction(tr("Find Next"),
                                             this, SLOT(findNextSearchMatch()),
                                             QKeySequence::FindNext);
    findPreviousMenuAction = editMenu->addAction(tr("Find Previous"),
                                                 this, SLOT(findPreviousSearchMatch()),
                                                 QKeySequence::FindPrevious);
    findNextMenuAction->setEnabled(false);
    findPreviousMenuAction->setEnabled(false);

    QMenu *formattingMenu = new QMenu(tr("F&ormatting"), this);
    menuBar()->addMenu(formattingMenu);
    formattingMenu->addAction(tr("Emphasized"), this, SLOT(formatSelectionEmphasized()),
                              QKeySequence("Ctrl+I"));
    formattingMenu->addAction(tr("Strong"), this, SLOT(formatSelectionStrong()),
                              QKeySequence("Ctrl+B"));
    formattingMenu->addAction(tr("Code"), this, SLOT(formatSelectionCode()),
                              QKeySequence("Ctrl+D"));

    QMenu *toolsMenu = new QMenu(tr("&Tools"), this);
    menuBar()->addMenu(toolsMenu);
    toolsMenu->addAction(tr("Increase Font Size"), this, SLOT(increaseFontSize()),
                         QKeySequence("Ctrl++"));
    toolsMenu->addAction(tr("Decrease Font Size"), this, SLOT(decreaseFontSize()),
                         QKeySequence("Ctrl+-"));
    toolsMenu->addAction(tr("&Preferences..."), this, SLOT(showPreferences()),
                         QKeySequence::Preferences);

    QMenu *compilingMenu = new QMenu(tr("&Compiling"), this);
    menuBar()->addMenu(compilingMenu);
    compilingMenu->addAction(tr("Compile to temporary HTML file"),
                             this, SLOT(compileToTempHTML()));
    compilingMenu->addAction(tr("Compile to HTML file..."),
                             this, SLOT(compileToHTMLAs()));
    recompileAction = compilingMenu->addAction(tr("Recompile"),
                                               this, SLOT(recompileToHTML()),
                                               QKeySequence("Ctrl+Return"));
    recompileAction->setEnabled(false);

    QMenu *helpMenu = new QMenu(tr("&Help"), this);
    menuBar()->addMenu(helpMenu);
    helpMenu->addAction(tr("About QarkDown"), this, SLOT(about()));
    helpMenu->addAction(tr("Check for Updates..."), this, SLOT(checkForUpdates()));

    updateRecentFilesMenu();
}

void MainWindow::performStartupTasks()
{
    bool rememberLastFile = settings->value(SETTING_REMEMBER_LAST_FILE,
                                            QVariant(DEF_REMEMBER_LAST_FILE)).toBool();
    if (rememberLastFile && settings->contains(SETTING_LAST_FILE))
        openFile(settings->value(SETTING_LAST_FILE).toString());

    //connect(qApp, SIGNAL(saveStateRequest(QSessionManager&)),
    //        this, SLOT(commitDataHandler(QSessionManager&)), Qt::DirectConnection);
    connect(qApp, SIGNAL(commitDataRequest(QSessionManager&)),
            this, SLOT(commitDataHandler(QSessionManager&)), Qt::DirectConnection);
    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(aboutToQuitHandler()), Qt::DirectConnection);

    connect(editor->document(), SIGNAL(contentsChange(int,int,int)),
            this, SLOT(handleContentsChange(int,int,int)));
    connect(preferencesDialog, SIGNAL(updated()),
            this, SLOT(preferencesUpdated()));
    connect(editor, SIGNAL(anchorClicked(QUrl)),
            this, SLOT(anchorClicked(QUrl)));
}

void MainWindow::reportStyleParsingErrors(QList<QPair<int, QString> > *list)
{
    QString msg;
    for (int i = 0; i < list->size(); i++)
        msg += QString("-- Line %1: %2\n").arg(list->at(i).first).arg(list->at(i).second);
    QMessageBox::warning(this, "Errors in parsing style", msg);
}

void MainWindow::anchorClicked(const QUrl &link)
{
    QDesktopServices::openUrl(link);
}


bool MainWindow::confirmQuit(bool interactionAllowed)
{
    if (!isDirty())
        return true;

    if (!interactionAllowed)
    {
        Logger::debug("interaction not allowed -- saving.");
        saveFile();
        return true;
    }

    Logger::debug("allows interaction.");

    QString fileBaseName = kUntitledFileUIName;
    bool weHaveSavePath = false;
    if (!openFilePath.isNull())
    {
        fileBaseName = QFileInfo(openFilePath).fileName();
        weHaveSavePath = true;
    }

    QMessageBox saveConfirmMessageBox(this);
    saveConfirmMessageBox.setWindowModality(Qt::WindowModal);
    saveConfirmMessageBox.setIcon(QMessageBox::Warning);
    saveConfirmMessageBox.setText(tr("Do you want to save the changes you made in the document “%1”?").arg(fileBaseName));
    saveConfirmMessageBox.setInformativeText(tr("Your changes will be lost if you don’t save them."));
    saveConfirmMessageBox.setDefaultButton(saveConfirmMessageBox.addButton(weHaveSavePath ? "Save" : "Save...", QMessageBox::AcceptRole));
    saveConfirmMessageBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    saveConfirmMessageBox.addButton(tr("Don’t Save"), QMessageBox::DestructiveRole);
    saveConfirmMessageBox.exec();

    switch (saveConfirmMessageBox.buttonRole(saveConfirmMessageBox.clickedButton()))
    {
        case QMessageBox::AcceptRole:
            saveFile();
            break;
        case QMessageBox::DestructiveRole:
            break;
        case QMessageBox::RejectRole:
        default:
            return false;
    }
    return true;
}

void MainWindow::commitDataHandler(QSessionManager &manager)
{
    Logger::debug("commitDataHandler.");

    bool interactionAllowed = manager.allowsInteraction();
    bool okToQuit = confirmQuit(interactionAllowed);
    if (interactionAllowed)
        manager.release();
    if (!okToQuit)
        manager.cancel();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool okToQuit = confirmQuit(true);
    if (okToQuit)
        event->accept();
    else
        event->ignore();
}

#ifdef QT_MAC_USE_COCOA
void MainWindow::cocoaCommitDataHandler()
{
    Logger::debug("cocoaCommitDataHandler.");

    bool okToQuit = confirmQuit(true);
    if (okToQuit)
    {
        [[NSApp delegate] performSelector:@selector(acceptPendingTermination)];
        qApp->quit();
    }
    else
        [[NSApp delegate] performSelector:@selector(cancelPendingTermination)];
}
#endif

void MainWindow::quitActionHandler()
{
    Logger::debug("quitActionHandler.");

    bool okToQuit = confirmQuit(true);
    if (okToQuit)
        qApp->quit();
}

void MainWindow::aboutToQuitHandler()
{
    // No user interaction allowed here
    bool rememberWindow = settings->value(SETTING_REMEMBER_WINDOW,
                                          QVariant(DEF_REMEMBER_WINDOW)).toBool();
    if (rememberWindow) {
        settings->setValue(SETTING_WINDOW_GEOMETRY, saveGeometry());
        settings->setValue(SETTING_WINDOW_STATE, saveState());
    }
    settings->sync();

    // If we still have uncommitted changes at this point, just
    // play it safe and save them:
    if (isDirty())
        saveFile();
}

void MainWindow::handleContentsChange(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position); Q_UNUSED(charsRemoved); Q_UNUSED(charsAdded);
    setDirty(true);
}
