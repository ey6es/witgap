//
// $Id$

#include <QKeyEvent>
#include <QMouseEvent>

#include "Protocol.h"
#include "ui/TextField.h"

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

TextField::TextField (int minWidth, Document* document, bool rightAlign, QObject* parent) :
    Component(parent),
    _minWidth(minWidth),
    _document(document),
    _rightAlign(rightAlign),
    _documentPos(0),
    _cursorPos(document->text().length())
{
    connect(this, SIGNAL(enterPressed()), SLOT(transferFocus()));
    connect(this, SIGNAL(textFull()), SLOT(transferFocus()));
}

TextField::TextField (int minWidth, const QString& text, bool rightAlign, QObject* parent) :
    Component(parent),
    _minWidth(minWidth),
    _document(new Document(text)),
    _rightAlign(rightAlign),
    _documentPos(0),
    _cursorPos(text.length())
{
    connect(this, SIGNAL(enterPressed()), SLOT(transferFocus()));
    connect(this, SIGNAL(textFull()), SLOT(transferFocus()));
}

TextField::~TextField ()
{
    delete _document;
}

void TextField::setText (const QString& text)
{
    _document->remove(0, _document->text().length());
    _document->insert(0, text);

    // "set" the document to update the positions and dirty
    setDocument(_document);
}

void TextField::setDocument (Document* document)
{
    _document = document;
    _documentPos = 0;
    _cursorPos = document->text().length();
    if (!updateDocumentPos()) {
        Component::dirty();
    }
}

void TextField::setCursorPosition (int pos)
{
    if (_cursorPos != pos) {
        int opos = _cursorPos;
        _cursorPos = pos;
        if (!updateDocumentPos()) {
            dirty(_cursorPos, 1);
            dirty(opos, 1);
        }
    }
}

void TextField::setLabel (const QString& label)
{
    if (_label != label) {
        _label = label;
        Component::dirty();
    }
}

QSize TextField::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, _minWidth + 2), qMax(hhint, 1));
}

void TextField::validate ()
{
    updateDocumentPos();
}

void TextField::draw (DrawContext* ctx) const
{
    Component::draw(ctx);

    // find the area within the margins
    int x = _margins.left() + 1, y = _margins.top(), width = textAreaWidth();

    // draw the brackets
    int flags = _enabled ? 0 : DIM_FLAG;
    ctx->drawChar(x - 1, y, '[' | flags);
    ctx->drawChar(x + width, y, ']' | flags);

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
            drawText(ctx, x, y, _documentPos, qMin(length - _documentPos, width), flags);
        }
    }

    // draw the cursor if in focus
    if (_focused) {
        if (_rightAlign && length < width) {
            ctx->drawChar(x + width - length - 1 + _cursorPos, y,
                REVERSE_FLAG | (_cursorPos < length ? cursorChar() : ' '));
        } else {
            ctx->drawChar(x + (_cursorPos - _documentPos), y,
                REVERSE_FLAG | (_cursorPos < length ? cursorChar() : ' '));
        }
    }
}

void TextField::drawText (DrawContext* ctx, int x, int y, int idx, int length, int flags) const
{
    ctx->drawString(x, y, _document->text().constData() + idx, length, flags);
}

int TextField::cursorChar () const
{
    return _document->text().at(_cursorPos).unicode();
}

void TextField::focusInEvent (QFocusEvent* e)
{
    // if we're going to hide the label, we need to dirty the entire component
    int dlength = _document->text().length();
    if ((!_label.isEmpty() && dlength == 0) || (_rightAlign && dlength < textAreaWidth())) {
        Component::dirty();
    } else {
        dirty(_cursorPos, 1);
    }
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

void TextField::keyPressEvent (QKeyEvent* e)
{
    QString text = e->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        int olen = _document->text().length();
        _document->insert(_cursorPos, text);
        int delta = _document->text().length() - olen;
        if (delta > 0) {
            _cursorPos += delta;
            if (!updateDocumentPos()) {
                dirty(_cursorPos - delta);
            }
            emit textChanged();
            if (_document->full()) {
                emit textFull();
            }
        }
        return;
    }
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
        } else {
            Component::keyPressEvent(e);
        }
        return;

    } else if (modifiers != Qt::ShiftModifier && modifiers != Qt::NoModifier) {
        Component::keyPressEvent(e);
        return;
    }
    switch (key) {
        case Qt::Key_Left:
            if (_cursorPos > 0) {
                _cursorPos--;
                if (!updateDocumentPos()) {
                    dirty(_cursorPos, 2);
                }
            }
            break;

        case Qt::Key_Right:
            if (_cursorPos < _document->text().length()) {
                _cursorPos++;
                if (!updateDocumentPos()) {
                    dirty(_cursorPos - 1, 2);
                }
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
                _document->remove(--_cursorPos, 1);
                if (!updateDocumentPos()) {
                    dirty(_cursorPos);
                }
                emit textChanged();
            }
            break;

        case Qt::Key_Delete:
            if (_cursorPos < _document->text().length()) {
                _document->remove(_cursorPos, 1);
                if (!updateDocumentPos()) {
                    dirty(_cursorPos);
                }
                emit textChanged();
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

void TextField::dirty (int idx)
{
    // dirty the lesser of the remaining document (plus one for a
    // possibly deleted character) and what's visible
    int dlength = _document->text().length();
    int width = textAreaWidth();
    if (_rightAlign && dlength < width) {
        dirty(-1, idx + 2);
    } else {
        dirty(idx, qMin(dlength - idx + 2, width - idx + _documentPos));
    }
}

void TextField::dirty (int idx, int length)
{
    int dlength = _document->text().length();
    int width = textAreaWidth();
    if (_rightAlign && dlength < width) {
        Component::dirty(QRect(_margins.left() + width - dlength + idx,
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

int PasswordField::cursorChar () const
{
    return '*';
}
