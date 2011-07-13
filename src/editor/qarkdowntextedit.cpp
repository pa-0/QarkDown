#include "qarkdowntextedit.h"

#include <QDebug>
#include <QKeyEvent>
#include <QTextBlock>
#include <QTextLayout>
#include <QApplication>
#include <QToolTip>

QarkdownTextEdit::QarkdownTextEdit(QWidget *parent) :
    LineNumberingPlainTextEdit(parent)
{
    this->setUndoRedoEnabled(true);
    this->setMouseTracking(true);

    _indentString = "    ";
    _spacesIndentWidthHint = 4;
    _anchorClickKeyModifiers = Qt::NoModifier;
    _highlightCurrentLine = true;
    _lineHighlightColor = DEF_LINE_HIGHLIGHT_COLOR;

    connect(this, SIGNAL(cursorPositionChanged()),
            this, SLOT(applyHighlightingToCurrentLine()));
    applyHighlightingToCurrentLine();
}

QarkdownTextEdit::~QarkdownTextEdit()
{
}

QString QarkdownTextEdit::indentString()
{
    return _indentString;
}
void QarkdownTextEdit::setIndentString(QString value)
{
    _indentString = value;
}

int QarkdownTextEdit::spacesIndentWidthHint()
{
    return _spacesIndentWidthHint;
}
void QarkdownTextEdit::setSpacesIndentWidthHint(int value)
{
    _spacesIndentWidthHint = value;
}

Qt::KeyboardModifiers QarkdownTextEdit::anchorClickKeyboardModifiers()
{
    return _anchorClickKeyModifiers;
}
void QarkdownTextEdit::setAnchorClickKeyboardModifiers(Qt::KeyboardModifiers value)
{
    _anchorClickKeyModifiers = value;
}

bool QarkdownTextEdit::highlightCurrentLine()
{
    return _highlightCurrentLine;
}
void QarkdownTextEdit::setHighlightCurrentLine(bool value)
{
    _highlightCurrentLine = value;

    if (_highlightCurrentLine)
        applyHighlightingToCurrentLine();
    else
        removeCurrentLineHighlighting();
}

QColor QarkdownTextEdit::currentLineHighlightColor()
{
    return _lineHighlightColor;
}

void QarkdownTextEdit::setCurrentLineHighlightColor(QColor value)
{
    _lineHighlightColor = (value.isValid()) ? value : DEF_LINE_HIGHLIGHT_COLOR;
}


bool QarkdownTextEdit::isBorderChar(QChar character)
{
    return (character.isNull()
            || character.unicode() == QChar::ParagraphSeparator
            || character.unicode() == QChar::LineSeparator
            );
}

bool QarkdownTextEdit::cursorIsBeforeLineContentStart(QTextCursor cursor)
{
    int curPos = cursor.position();
    QChar character;
    int i = 1;
    do {
        character = this->document()->characterAt(curPos - i);
        if (isBorderChar(character))
            return true;
        i++;
    } while (character == ' ' || character == '\t');
    return false;
}

bool QarkdownTextEdit::selectionContainsOnlyFullLines(QTextCursor selection)
{
    QChar startChar = this->document()->characterAt(selection.selectionStart()-1);
    QChar lastChar = this->document()->characterAt(selection.selectionEnd()-1);
    QChar afterLastChar = this->document()->characterAt(selection.selectionEnd());
    bool startsAtLineStart = isBorderChar(startChar);
    bool endsAtLineEnd = (afterLastChar.isNull()
                          || lastChar.unicode() == QChar::ParagraphSeparator
                          || lastChar.unicode() == QChar::LineSeparator
                          );
    return (startsAtLineStart && endsAtLineEnd);
}

QList<int> QarkdownTextEdit::getLineStartPositionsInSelection(QTextCursor selection)
{
    QList<int> lineStarts;
    QTextCursor c(document());
    c.setPosition(selection.selectionStart());
    QTextBlock b = c.block();
    int pos = b.position();
    while (pos < selection.selectionEnd())
    {
        lineStarts.append(pos);
        b = b.next();
        if (!b.isValid())
            break;
        pos = b.position();
    }
    return lineStarts;
}


bool QarkdownTextEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        // Indenting with tab/backtab
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab)
        {
            QTextCursor cursor = this->textCursor();
            if (!cursor.hasSelection())
            {
                if (ke->key() == Qt::Key_Tab)
                    indentAtCursor();
                else // backtab
                    unindentAtCursor();
                return true;
            }
            else if (!cursor.hasComplexSelection())
            {
                // There is a non-complex selection.

                if (!selectionContainsOnlyFullLines(cursor)) {
                    cursor.clearSelection();
                    setTextCursor(cursor);
                    return true;
                }

                if (ke->key() == Qt::Key_Tab)
                    indentSelectedLines();
                else // backtab
                    unindentSelectedLines();
                return true;
            }
        }
    }
    else if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent *he = static_cast<QHelpEvent *>(e);

        QString href = getAnchorHrefAtPos(he->pos());
        if (!href.isNull())
            QToolTip::showText(he->globalPos(), href);
        else {
            QToolTip::hideText();
            e->ignore();
        }
        return true;
    }
    return LineNumberingPlainTextEdit::event(e);
}

QString QarkdownTextEdit::getAnchorHrefAtPos(QPoint pos)
{
    QTextCursor cur = cursorForPosition(pos);
    QList<QTextLayout::FormatRange> formats = cur.block().layout()->additionalFormats();
    int posInBlock = cur.position() - cur.block().position();
    foreach (QTextLayout::FormatRange range, formats)
    {
        if (posInBlock < range.start || range.start + range.length < posInBlock)
            continue;
        QString href = range.format.anchorHref();
        if (!href.isNull())
            return href;
    }
    return QString();
}


void QarkdownTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->modifiers() & _anchorClickKeyModifiers) == _anchorClickKeyModifiers)
    {
        QString href = getAnchorHrefAtPos(e->pos());
        if (href.isNull())
            viewport()->setCursor(Qt::IBeamCursor);
        else
            viewport()->setCursor(Qt::PointingHandCursor);
    }
    else
        viewport()->setCursor(Qt::IBeamCursor);
    LineNumberingPlainTextEdit::mouseMoveEvent(e);
}

void QarkdownTextEdit::mousePressEvent(QMouseEvent *e)
{
    if ((e->modifiers() & _anchorClickKeyModifiers) == _anchorClickKeyModifiers)
    {
        // The caret is placed upon press (not release) so we disable
        // that here if the mouse is pressed on an anchor:
        QString href = getAnchorHrefAtPos(e->pos());
        if (!href.isNull()) {
            e->ignore();
            return;
        }
    }
    LineNumberingPlainTextEdit::mouseReleaseEvent(e);
}

void QarkdownTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if ((e->modifiers() & _anchorClickKeyModifiers) == _anchorClickKeyModifiers)
    {
        QString href = getAnchorHrefAtPos(e->pos());
        if (!href.isNull()) {
            emit anchorClicked(QUrl(href));
            e->ignore();
            return;
        }
    }
    LineNumberingPlainTextEdit::mouseReleaseEvent(e);
}


int QarkdownTextEdit::guessNumOfSpacesToDeleteUponUnindenting()
{
    int spacesToDelete = _spacesIndentWidthHint;
    if (spacesToDelete == 0 && _indentString.startsWith(" "))
        spacesToDelete = _indentString.length();
    return spacesToDelete;
}

void QarkdownTextEdit::indentSelectedLines()
{
    QTextCursor cursor = this->textCursor();

    QList<int> lineStarts = getLineStartPositionsInSelection(cursor);

    // Insert indentString to line start positions
    QTextCursor insertCursor(document());
    insertCursor.beginEditBlock();
    int shift = 0;
    foreach (int lineStart, lineStarts)
    {
        insertCursor.setPosition(lineStart+shift);
        insertCursor.insertText(_indentString);
        shift += _indentString.length();
    }
    insertCursor.endEditBlock();

    if (cursor.hasSelection())
    {
        // Adjust selection to include first added indentString
        int selEnd = cursor.selectionEnd();
        cursor.setPosition(cursor.selectionStart()-_indentString.length());
        cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
        this->setTextCursor(cursor);
    }
}

void QarkdownTextEdit::unindentSelectedLines()
{
    QTextCursor cursor = this->textCursor();

    QList<int> lineStarts = getLineStartPositionsInSelection(cursor);

    QTextCursor removalCursor(document());
    removalCursor.beginEditBlock();
    int deletedChars = 0;
    foreach (int lineStart, lineStarts)
    {
        int adjustedStart = lineStart - deletedChars;
        removalCursor.setPosition(adjustedStart);
        if (document()->characterAt(adjustedStart) == QChar('\t'))
        {
            // line starts with tab -> just delete the tab.
            removalCursor.deleteChar();
            deletedChars++;
        }
        else if (document()->characterAt(adjustedStart) == QChar(' '))
        {
            // line starts with a space -> must guess how many spaces to delete.
            int spacesToDelete = guessNumOfSpacesToDeleteUponUnindenting();

            while (spacesToDelete > 0 && document()->characterAt(adjustedStart) == QChar(' '))
            {
                removalCursor.deleteChar();
                deletedChars++;
                spacesToDelete--;
            }
        }
    }
    removalCursor.endEditBlock();
}

void QarkdownTextEdit::indentAtCursor()
{
    QTextCursor cursor = this->textCursor();
    QTextCursor insertCursor(document());
    insertCursor.beginEditBlock();
    insertCursor.setPosition(cursor.position());
    insertCursor.insertText(_indentString);
    insertCursor.endEditBlock();
}

void QarkdownTextEdit::unindentAtCursor()
{
    QTextCursor cursor = this->textCursor();
    QTextCursor removalCursor(document());
    removalCursor.beginEditBlock();

    QTextBlock b = cursor.block();
    int lineStartPos = b.position();
    removalCursor.setPosition(lineStartPos);
    if (document()->characterAt(lineStartPos) == QChar('\t'))
    {
        // line starts with tab -> just delete the tab.
        removalCursor.deleteChar();
    }
    else if (document()->characterAt(lineStartPos) == QChar(' '))
    {
        // line starts with a space -> must guess how many spaces to delete.
        int spacesToDelete = guessNumOfSpacesToDeleteUponUnindenting();

        while (spacesToDelete > 0 && document()->characterAt(lineStartPos) == QChar(' '))
        {
            removalCursor.deleteChar();
            spacesToDelete--;
        }
    }

    removalCursor.endEditBlock();
}



void QarkdownTextEdit::applyHighlightingToCurrentLine()
{
    if (!_highlightCurrentLine)
        return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QTextCursor selCur(textCursor());
        QTextBlock b = selCur.block();
        selCur.setPosition(b.position());
        selCur.setPosition(b.position()+b.length()-1, QTextCursor::KeepAnchor);

        // highlight only if line is not empty
        if (selCur.selectionStart() < selCur.selectionEnd())
        {
            selection.format.setBackground(_lineHighlightColor);
            selection.cursor = selCur;
            extraSelections.append(selection);
        }
    }

    setExtraSelections(extraSelections);
}

void QarkdownTextEdit::removeCurrentLineHighlighting()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    setExtraSelections(extraSelections);
}
