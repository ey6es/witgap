//
// $Id$

#include <limits>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QtDebug>

#include "Protocol.h"
#include "ui/Border.h"
#include "ui/TextComponent.h"

using namespace std;

Document::Document (const QString& text, int maxLength) :
    _text(text),
    _maxLength(maxLength)
{
}

void Document::insert (int idx, const QString& text)
{
    if (_text.length() + text.length() <= _maxLength) {
        _text.insert(idx, text);
    }
}

void Document::remove (int idx, int length)
{
    _text.remove(idx, length);
}

RegExpDocument::RegExpDocument (const QRegExp& regExp, const QString& text, int maxLength) :
    Document(text, maxLength),
    _regExp(regExp)
{
}

void RegExpDocument::insert (int idx, const QString& text)
{
    QString ntext = _text;
    ntext.insert(idx, text);
    if (_regExp.exactMatch(ntext) && ntext.length() <= _maxLength) {
        _text = ntext;
    }
}

TextComponent::TextComponent (int minWidth, Document* document, QObject* parent) :
    Component(parent),
    _minWidth(minWidth),
    _document(document),
    _documentPos(0),
    _cursorPos(document->text().length()),
    _matchParentheses(false),
    _matchPos(-1)
{
    connect(&_matchTimer, SIGNAL(timeout()), SLOT(clearMatch()));
    _matchTimer.setSingleShot(true);
}

TextComponent::~TextComponent ()
{
    delete _document;
}

void TextComponent::setText (const QString& text)
{
    _document->remove(0, _document->text().length());
    _document->insert(0, text);

    // "set" the document to update the positions and dirty
    setDocument(_document);
}

void TextComponent::setCursorPosition (int pos)
{
    if (_cursorPos != pos) {
        int opos = _cursorPos;
        _cursorPos = pos;
        if (!updateDocumentPos()) {
            dirty(_cursorPos, 1);
            dirty(opos, 1);
        }
        maybeShowMatch();
    }
}

void TextComponent::setLabel (const QString& label)
{
    if (_label != label) {
        _label = label;
        Component::dirty();
    }
}

void TextComponent::setMatchParentheses (bool match)
{
    _matchParentheses = match;
}

void TextComponent::validate ()
{
    updateDocumentPos();
}

void TextComponent::drawText (DrawContext* ctx, int x, int y, int idx, int length, int flags) const
{
    ctx->drawString(x, y, _document->text().constData() + idx, length, flags);
}

int TextComponent::visibleChar (int idx) const
{
    return _document->text().at(idx).unicode();
}

void TextComponent::keyPressEvent (QKeyEvent* e)
{
    int key = e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers == Qt::ControlModifier) {
        int opos = _cursorPos;
        if (key == Qt::Key_Left) { // previous word beginning
            const QString& text = _document->text();
            if (_cursorPos > 0) {
                for (int idx = _cursorPos - 1; idx > 0; idx--) {
                    if (text.at(idx).isLetterOrNumber() && !text.at(idx - 1).isLetterOrNumber()) {
                        setCursorPosition(idx);
                        break;
                    }
                }
                if (_cursorPos == opos) {
                    setCursorPosition(0);
                }
            }
        } else if (key == Qt::Key_Right) { // next word end
            const QString& text = _document->text();
            if (_cursorPos < text.length()) {
                for (int idx = _cursorPos + 1, max = text.length(); idx < max; idx++) {
                    if (!text.at(idx).isLetterOrNumber() && text.at(idx - 1).isLetterOrNumber()) {
                        setCursorPosition(idx);
                        break;
                    }
                }
                if (_cursorPos == opos) {
                    setCursorPosition(text.length());
                }
            }
        } else if (key == Qt::Key_Z) {
            _undoStack.undo();

        } else if (key == Qt::Key_Y) {
            _undoStack.redo();

        } else {
            Component::keyPressEvent(e);
        }
        return;

    } else if (modifiers == (Qt::ControlModifier | Qt::ShiftModifier) && key == Qt::Key_Z) {
        _undoStack.redo();
        return;

    } else if (modifiers != Qt::ShiftModifier && modifiers != Qt::NoModifier) {
        Component::keyPressEvent(e);
        return;
    }
    QString text = e->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        _undoStack.push(new TextEditCommand(this, _cursorPos, text));
        return;
    }
    switch (key) {
        case Qt::Key_Left:
            if (_cursorPos > 0) {
                _cursorPos--;
                if (!updateDocumentPos()) {
                    dirty(_cursorPos, 2);
                }
                maybeShowMatch();
            }
            break;

        case Qt::Key_Right:
            if (_cursorPos < _document->text().length()) {
                _cursorPos++;
                if (!updateDocumentPos()) {
                    dirty(_cursorPos - 1, 2);
                }
                maybeShowMatch();
            }
            break;

        case Qt::Key_Home:
            setCursorPosition(0);
            break;

        case Qt::Key_End:
            setCursorPosition(_document->text().length());
            break;

        case Qt::Key_Backspace:
            if (_cursorPos > 0) {
                _undoStack.push(new TextEditCommand(this, _cursorPos - 1, 1, true));
            }
            break;

        case Qt::Key_Delete:
            if (_cursorPos < _document->text().length()) {
                _undoStack.push(new TextEditCommand(this, _cursorPos, 1, false));
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            emit enterPressed();
            break;

        default:
            Component::keyPressEvent(e);
            break;
    }
}

void TextComponent::maybeShowMatch ()
{
    clearMatch();
    if (!_matchParentheses) {
        return;
    }
    if (!(_cursorPos < _document->text().length() && maybeShowMatch(_cursorPos)) &&
            _cursorPos > 0) {
        maybeShowMatch(_cursorPos - 1);
    }
}

bool TextComponent::maybeShowMatch (int idx)
{
    const QString& text = _document->text();
    QChar ch = text.at(idx);
    if (ch == '(' || ch == '[') {
        QChar open = ch, close = (ch == '(') ? ')' : ']';
        int depth = 1;
        for (int ii = idx + 1, nn = text.length(); ii < nn; ii++) {
            QChar ch = text.at(ii);
            if (ch == open) {
                depth++;

            } else if (ch == close && --depth == 0) {
                showMatch(ii);
                break;
            }
        }
        return true;

    } else if (ch == ')' || ch == ']') {
        QChar open = (ch == ')') ? '(' : '[', close = ch;
        int depth = 1;
        for (int ii = idx - 1; ii >= 0; ii--) {
            QChar ch = text.at(ii);
            if (ch == close) {
                depth++;

            } else if (ch == open && --depth == 0) {
                showMatch(ii);
                break;
            }
        }
        return true;
    }
    return false;
}

TextField::TextField (int minWidth, Document* document, bool rightAlign, QObject* parent) :
    TextComponent(minWidth, document, parent),
    _rightAlign(rightAlign)
{
    connect(this, SIGNAL(enterPressed()), SLOT(transferFocus()));
}

TextField::TextField (int minWidth, const QString& text, bool rightAlign, QObject* parent) :
    TextComponent(minWidth, new Document(text), parent),
    _rightAlign(rightAlign)
{
    connect(this, SIGNAL(enterPressed()), SLOT(transferFocus()));
}

void TextField::setDocument (Document* document)
{
    _document = document;
    _documentPos = 0;
    _cursorPos = document->text().length();
    if (!updateDocumentPos()) {
        Component::dirty();
    }
    _undoStack.clear();
    maybeShowMatch();
}

QSize TextField::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, _minWidth + 2), qMax(hhint, 1));
}

void TextField::draw (DrawContext* ctx)
{
    Component::draw(ctx);

    // find the area within the margins
    int x = _margins.left() + 1, y = _margins.top(), width = textAreaWidth();

    // draw the brackets
    int flags = _enabled ? 0 : DIM_FLAG;
    ctx->drawChar(x - 1, y, '[' | flags);
    ctx->drawChar(x + width, y, ']' | flags);

    // make sure the match is visible
    int pos = _documentPos;
    if (_focused && _matchPos != -1) {
        if (_matchPos < _documentPos) {
            pos = _matchPos;

        } else if (_matchPos >= _documentPos + width) {
            pos = _matchPos - width + 1;
        }
    }

    // draw the visible portion of the contents or the label, if appropriate
    int length = _document->text().length();
    if (length == 0) {
        int llength = _label.length();
        if (!_focused && llength > 0) {
            int dwidth = qMin(llength, width);
            ctx->drawString(x + (_rightAlign ? width - dwidth : 0), y,
                _label.constData(), dwidth, DIM_FLAG);
        }
    } else {
        if (_rightAlign && length < width) {
            // when focused, we allocate extra space for the cursor
            drawText(ctx, x + width - (_focused ? 1 : 0) - length, y, 0, length, flags);
        } else {
            drawText(ctx, x, y, pos, qMin(length - pos, width), flags);
        }
    }

    // draw the cursor and match if in focus
    if (_focused) {
        if (_rightAlign && length < width) {
            ctx->drawChar(x + width - length - 1 + _cursorPos, y,
                REVERSE_FLAG | (_cursorPos < length ? visibleChar(_cursorPos) : ' '));

        } else if (_cursorPos >= pos && _cursorPos < pos + width) {
            ctx->drawChar(x + (_cursorPos - pos), y,
                REVERSE_FLAG | (_cursorPos < length ? visibleChar(_cursorPos) : ' '));
        }

        // draw the match if appropriate
        if (_matchPos != -1) {
            if (_rightAlign && length < width) {
                ctx->drawChar(x + width - length - 1 + _matchPos, y,
                    REVERSE_FLAG | visibleChar(_matchPos));

            } else {
                ctx->drawChar(x + (_matchPos - pos), y, REVERSE_FLAG | visibleChar(_matchPos));
            }
        }
    }
}

void TextField::focusInEvent (QFocusEvent* e)
{
    Component::focusInEvent(e);

    // if we're going to hide the label, we need to dirty the entire component
    int dlength = _document->text().length();
    if ((!_label.isEmpty() && dlength == 0) || (_rightAlign && dlength < textAreaWidth())) {
        Component::dirty();
    } else {
        dirty(_cursorPos, 1);
    }

    maybeShowMatch();
}

void TextField::focusOutEvent (QFocusEvent* e)
{
    // likewise if we're going to show the label
    int dlength = _document->text().length();
    if ((!_label.isEmpty() && dlength == 0) || (_rightAlign && dlength < textAreaWidth())) {
        Component::dirty();
    } else {
        dirty(_cursorPos, 1);
    }

    clearMatch();
}

void TextField::mouseButtonPressEvent (QMouseEvent* e)
{
    QRect inner = innerRect();
    inner.setHeight(1);
    inner.setX(inner.x() + 1);
    inner.setWidth(inner.width() - 1);
    if (inner.contains(e->pos())) {
        int length = _document->text().length();
        int fwidth = inner.width() - 1;
        if (_rightAlign && length < fwidth) {
            setCursorPosition(qMax(e->pos().x() - inner.x() - fwidth + length, 0));
        } else {
            setCursorPosition(qMin(_documentPos + e->pos().x() - inner.x(), length));
        }
    }
    Component::mouseButtonPressEvent(e);
}

QString TextField::insert (int idx, const QString& text, bool cursorAfter)
{
    int olen = _document->text().length();
    _document->insert(idx, text);
    int delta = _document->text().length() - olen;
    if (delta == 0) {
        return QString();
    }
    int opos = _cursorPos;
    _cursorPos = idx + (cursorAfter ? delta : 0);
    if (!updateDocumentPos()) {
        if (_rightAlign && olen + delta < textAreaWidth()) {
            dirtyRest(qMax(idx + delta - 1, opos + delta), 0);
        } else {
            dirtyRest(qMin(opos, idx), 0);
        }
    }
    emit textChanged();
    if (_document->full()) {
        emit textFull();
    }
    maybeShowMatch();
    return _document->text().mid(idx, delta);
}

void TextField::remove (int idx, int length)
{
    if (length == 0) {
        return;
    }
    int opos = _cursorPos;
    _document->remove(_cursorPos = idx, length);
    if (!updateDocumentPos()) {
        if (_rightAlign && _document->text().length() < textAreaWidth()) {
            dirtyRest(qMax(idx, opos - length), length);
        } else {
            dirtyRest(qMin(opos, _cursorPos), length);
        }
    }
    maybeShowMatch();
    emit textChanged();
}

/** The number of milliseconds for which we show a parenthesis match. */
static const int MatchDuration = 1000;

void TextField::showMatch (int idx)
{
    _matchPos = idx;
    _matchTimer.start(MatchDuration);
    if (_matchPos >= _documentPos && _matchPos < _documentPos + textAreaWidth()) {
        dirty(_matchPos, 1);
    } else {
        Component::dirty();
    }
}

void TextField::clearMatch ()
{
    if (_matchPos != -1) {
        if (_matchPos >= _documentPos && _matchPos < _documentPos + textAreaWidth()) {
            dirty(_matchPos, 1);
        } else {
            Component::dirty();
        }
        _matchPos = -1;
    }
    _matchTimer.stop();
}

bool TextField::updateDocumentPos ()
{
    int width = textAreaWidth() - 1;
    int opos = _documentPos;
    _documentPos = qBound(qMax(_cursorPos - width, 0), _documentPos,
        qMin(_cursorPos, _document->text().length() - width));
    if (opos != _documentPos) {
        Component::dirty();
        return true;
    }
    return false;
}

void TextField::dirtyRest (int idx, int deleted)
{
    // dirty the lesser of the remaining document (plus any deleted characters) and what's visible
    int dlength = _document->text().length();
    int width = textAreaWidth();
    if (_rightAlign && dlength < width) {
        dirty(-deleted, idx + deleted + 1);
    } else {
        dirty(idx, qMin(dlength - idx + deleted + 1, width - idx + _documentPos));
    }
}

void TextField::dirty (int idx, int length)
{
    int dlength = _document->text().length();
    int width = textAreaWidth();
    if (_rightAlign && dlength < width) {
        Component::dirty(QRect(_margins.left() + 1 + width - (_focused ? 1 : 0) - dlength + idx,
            _margins.top(), length, 1));
    } else {
        Component::dirty(QRect(_margins.left() + 1 + idx - _documentPos,
            _margins.top(), length, 1));
    }
}

int TextField::textAreaWidth () const
{
    return _bounds.width() - _margins.left() - _margins.right() - 2;
}

PasswordField::PasswordField (int minWidth, Document* document, QObject* parent) :
    TextField(minWidth, document, parent)
{
}

PasswordField::PasswordField (int minWidth, const QString& text, QObject* parent) :
    TextField(minWidth, text, parent)
{
}

void PasswordField::drawText (DrawContext* ctx, int x, int y, int idx, int length, int flags) const
{
    ctx->fillRect(x, y, length, 1, '*' | flags);
}

int PasswordField::visibleChar (int idx) const
{
    return '*';
}

Span::Span (int start, int length) :
    start(start),
    length(length)
{
}

TextArea::TextArea (int minWidth, int minHeight, Document* document,
        Wrap wrap, QObject* parent) :
    TextComponent(minWidth, document, parent),
    _minHeight(minHeight),
    _wrap(wrap)
{
    setBorder(new FrameBorder());
}

TextArea::TextArea (int minWidth, int minHeight, const QString& text,
        Wrap wrap, QObject* parent) :
    TextComponent(minWidth, new Document(text, 1024), parent),
    _minHeight(minHeight),
    _wrap(wrap)
{
    setBorder(new FrameBorder());
}

void TextArea::setWrap (Wrap wrap)
{
    if (_wrap != wrap) {
        _wrap = wrap;
        invalidate();
    }
}

void TextArea::setDocument (Document* document)
{
    _document = document;
    _documentPos = 0;
    _cursorPos = 0;
    invalidate();
    _undoStack.clear();
    maybeShowMatch();
}

QSize TextArea::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, _minWidth), qMax(hhint, _minHeight));
}

void TextArea::validate ()
{
    // break the text up into lines and append to the list
    int length = 0;
    int wlimit = (_wrap == NoWrap) ? numeric_limits<int>::max() : innerRect().width() - 1;
    const QChar* start = _document->text().constData(), *whitespace = 0;
    bool wrun = false;
    _lines.resize(0);
    _lineIndices.resize(0);
    for (const QChar* ptr = start, *end = ptr + _document->text().size(); ptr < end; ptr++) {
        QChar value = *ptr;
        if (value != '\n') {
            if (value == ' ' && _wrap == WordWrap) {
                if (!wrun) { // it's the start of a run of whitespace
                    wrun = true;
                    whitespace = ptr;
                }
            } else {
                wrun = false;
            }
            if (++length <= wlimit) {
                _lineIndices.append(_lines.size());
                continue;
            }
            if (whitespace == 0) { // nowhere to break
                length--;

            } else {
                int remove = ptr - whitespace;
                length -= remove;
                _lineIndices.remove(_lineIndices.size() - remove, remove);

                // consume any whitespace after the line
                for (ptr = whitespace; ptr < end && *ptr == ' '; ptr++) {
                    _lineIndices.append(_lines.size());
                }
            }
            ptr--;
        
        } else {
            length++;
            _lineIndices.append(_lines.size());    
        }
        _lines.append(Span(start - _document->text().constData(), length));
        start = ptr + 1;
        length = 0;
        whitespace = 0;
        wrun = false;
    }
    _lines.append(Span(start - _document->text().constData(), length));
}

void TextArea::draw (DrawContext* ctx)
{
    Component::draw(ctx);
    
    // find the area within the margins
    QRect inner = innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width(), height = inner.height();
    
    // draw as many lines as are visible
    int flags = _enabled ? 0 : DIM_FLAG;
    int documentLine = getLineIndex(_documentPos);
    int cursorLine = getLineIndex(_cursorPos);
    const Span& dspan = _lines.at(documentLine);
    int offset = _documentPos - dspan.start;
    for (int ii = documentLine, nn = _lines.size(); ii < nn && height > 0; ii++, height--, y++) {
        const Span& line = _lines.at(ii);
        if (line.length > offset) {
            drawText(ctx, x, y, line.start + offset, qMin(width, line.length - offset), flags);
        }
        if (ii == cursorLine && _focused) {
            ctx->drawChar(x + _cursorPos - line.start - offset, y,
                visibleChar(_cursorPos) | REVERSE_FLAG);
        }
    }
}

void TextArea::focusInEvent (QFocusEvent* e)
{
    Component::focusInEvent(e);

    // if we're going to hide the label, we need to dirty the entire component
    if (!_label.isEmpty() && _document->text().isEmpty()) {
        Component::dirty();
    } else {
        Component::dirty();
    }

    maybeShowMatch();
}

void TextArea::focusOutEvent (QFocusEvent* e)
{
    // likewise if we're going to show the label
    int dlength = _document->text().length();
    if (!_label.isEmpty() && _document->text().isEmpty()) {
        Component::dirty();
    } else {
        Component::dirty();
    }

    clearMatch();
}

void TextArea::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers(); 
    if (modifiers == Qt::ShiftModifier || modifiers == Qt::NoModifier) {
        switch (e->key()) {
            case Qt::Key_Up: {
                int cursorLine = getLineIndex(_cursorPos);
                if (cursorLine > 0) {
                    const Span& ospan = _lines.at(cursorLine);
                    const Span& nspan = _lines.at(cursorLine - 1);
                    setCursorPosition(nspan.start +
                        qMin(nspan.length - 1, _cursorPos - ospan.start));
                    return;
                }
                break;
            }
            case Qt::Key_Down: {
                int cursorLine = getLineIndex(_cursorPos);
                if (cursorLine < _lines.size() - 1) {
                    const Span& ospan = _lines.at(cursorLine);
                    const Span& nspan = _lines.at(cursorLine + 1);
                    setCursorPosition(nspan.start +
                        qMin(nspan.length - 1, _cursorPos - ospan.start));
                    return;
                }
                break;
            }
            case Qt::Key_PageUp: {
                int cursorLine = getLineIndex(_cursorPos);
                if (cursorLine > 0) {
                    const Span& ospan = _lines.at(cursorLine);
                    const Span& nspan = _lines.at(qMax(cursorLine - innerRect().height() + 1, 0));
                    setCursorPosition(nspan.start +
                        qMin(nspan.length - 1, _cursorPos - ospan.start));
                    return;
                }
                break;
            }
            case Qt::Key_PageDown: {
                int cursorLine = getLineIndex(_cursorPos);
                if (cursorLine < _lines.size() - 1) {
                    const Span& ospan = _lines.at(cursorLine);
                    const Span& nspan = _lines.at(qMin(cursorLine + innerRect().height() - 1,
                        _lines.size() - 1));
                    setCursorPosition(nspan.start +
                        qMin(nspan.length - 1, _cursorPos - ospan.start));
                    return;
                }
                break;
            }
            case Qt::Key_Home: {
                const Span& cspan = _lines.at(getLineIndex(_cursorPos));
                setCursorPosition(cspan.start);
                return;
            }
            case Qt::Key_End: {
                const Span& cspan = _lines.at(getLineIndex(_cursorPos));
                setCursorPosition(cspan.start + cspan.length - 1);
                return;
            }
        }
    }
    TextComponent::keyPressEvent(e);
}

void TextArea::mouseButtonPressEvent (QMouseEvent* e)
{
    QRect inner = innerRect();
    if (inner.contains(e->pos())) {
        int documentLine = getLineIndex(_documentPos);
        const Span& dspan = _lines.at(documentLine);
        int offset = _documentPos - dspan.start;
    
        int cursorLine = qMin(documentLine + e->pos().y() - inner.y(), _lines.size() - 1);
        const Span& cspan = _lines.at(cursorLine);
        setCursorPosition(qMin(cspan.start + offset + e->pos().x() - inner.x(),
            cspan.start + qMax(0, cspan.length - 1)));
    }
    Component::mouseButtonPressEvent(e);    
}

void TextArea::clearMatch ()
{
}

QString TextArea::insert (int idx, const QString& text, bool cursorAfter)
{
    int olen = _document->text().length();
    _document->insert(idx, text);
    int delta = _document->text().length() - olen;
    if (delta == 0) {
        return QString();
    }
    int opos = _cursorPos;
    _cursorPos = idx + (cursorAfter ? delta : 0);
    int lidx = getLineIndex(idx);
    _lineIndices.insert(idx, delta, lidx);
    _lines[lidx].length += delta;
    
    int wlimit = (_wrap == NoWrap) ? numeric_limits<int>::max() : innerRect().width() - 1;
    for (int nn = _lines.size(); lidx < nn; lidx++) {
        Span& line = _lines[lidx];
        if (line.length <= wlimit) {
            break;
        }
        int widx = line.start + wlimit;
        for (int ii = widx - 1; ii >= line.start; ii--) {
            if (_document->text().at(ii) == ' ') {
                widx = ii + 1;
                break;
            }
        }
        
    }
    
    if (!updateDocumentPos()) {
        Component::dirty();
    }
    maybeShowMatch();
    emit textChanged();
    return _document->text().mid(idx, delta);
}

void TextArea::remove (int idx, int length)
{
    if (length == 0) {
        return;
    }
    int opos = _cursorPos;
    _document->remove(_cursorPos = idx, length);
    int lidx = getLineIndex(idx);
    _lineIndices.remove(idx, length);
    
    
    
    if (!updateDocumentPos()) {
        Component::dirty();
    }
    maybeShowMatch();
    emit textChanged();
}

void TextArea::showMatch (int idx)
{
}

bool TextArea::updateDocumentPos ()
{
    QRect inner = innerRect();
    int cursorLine = getLineIndex(_cursorPos);
    const Span& cspan = _lines.at(cursorLine);
    int documentLine = getLineIndex(_documentPos);
    const Span& dspan = _lines.at(documentLine);
    if (cursorLine < documentLine) {
        _documentPos = cspan.start + qMin(cspan.length - 1, _documentPos - dspan.start);
    
    } else if (cursorLine >= documentLine + inner.height()) {
        const Span& nspan = _lines.at(cursorLine - inner.height() + 1);
        _documentPos = nspan.start + qMin(nspan.length - 1, _documentPos - dspan.start);
        
    } else if (_cursorPos - cspan.start < _documentPos - dspan.start) {
        _documentPos = dspan.start + (_cursorPos - cspan.start);
        
    } else if (_cursorPos - cspan.start >= _documentPos - dspan.start + inner.width()) {
        _documentPos = dspan.start + (_cursorPos - cspan.start - inner.width() + 1);
    
    } else {
        return false;
    }
    Component::dirty();
    return true;
}

void TextArea::dirty (int idx, int length)
{
    Component::dirty();
}

int TextArea::getLineIndex (int pos) const
{
    return pos < _lineIndices.size() ? _lineIndices.at(pos) : _lines.size() - 1;
}

FieldExpEnabler::FieldExpEnabler (
        Component* component, TextComponent* f1, const QRegExp& e1,
        TextComponent* f2, const QRegExp& e2, TextComponent* f3, const QRegExp& e3,
        TextComponent* f4, const QRegExp& e4) :
    QObject(component),
    _component(component)
{
    FieldExp fieldExps[] = { FieldExp(f1, e1), FieldExp(f2, e2),
        FieldExp(f3, e3), FieldExp(f4, e4) };
    for (int ii = 0; ii < 4; ii++) {
        if (fieldExps[ii].first != 0) {
            connect(fieldExps[ii].first, SIGNAL(textChanged()), SLOT(updateComponent()));
            _fieldExps.append(fieldExps[ii]);
        }
    }
    updateComponent();
}

void FieldExpEnabler::updateComponent ()
{
    foreach (const FieldExp& fieldExp, _fieldExps) {
        if (!fieldExp.second.exactMatch(fieldExp.first->text())) {
            _component->setEnabled(false);
            return;
        }
    }
    _component->setEnabled(true);
}

TextEditCommand::TextEditCommand (TextComponent* component, int idx, const QString& text) :
    _component(component),
    _insertion(true),
    _idx(idx),
    _text(text)
{
}

TextEditCommand::TextEditCommand (TextComponent* component, int idx, int length, bool backspace) :
    _component(component),
    _insertion(false),
    _backspace(backspace),
    _idx(idx),
    _text(component->text().mid(idx, length))
{
}

void TextEditCommand::undo ()
{
    apply(true);
}

void TextEditCommand::redo ()
{
    apply(false);
}

int TextEditCommand::id () const
{
    return 1;
}

bool TextEditCommand::mergeWith (const QUndoCommand* command)
{
    const TextEditCommand* ocmd = static_cast<const TextEditCommand*>(command);
    if (_insertion) {
        // we don't merge segments that end with whitespace with segments that begin with
        // non-whitespace
        int len = _text.length();
        if (ocmd->_insertion && _idx + len == ocmd->_idx &&
                (len == 0 || ocmd->_text.length() == 0 || !_text.at(len - 1).isSpace() ||
                    ocmd->_text.at(0).isSpace())) {
            _text.append(ocmd->_text);
            return true;
        }
    } else if (!ocmd->_insertion) {
        if (_backspace) {
            if (ocmd->_backspace && _idx == ocmd->_idx + ocmd->_text.length()) {
                _text.prepend(ocmd->_text);
                _idx = ocmd->_idx;
                return true;
            }
        } else if (!ocmd->_backspace && _idx == ocmd->_idx) {
            _text.append(ocmd->_text);
            return true;
        }
    }
    return false;
}

void TextEditCommand::apply (bool reverse)
{
    if (_insertion == reverse) {
        _component->remove(_idx, _text.length());

    } else {
        _text = _component->insert(_idx, _text, _insertion || _backspace);
    }
}
