//
// $Id$

#include "Protocol.h"
#include "ui/TextField.h"

Document::Document (const QString& text) :
    _text(text)
{
}

void Document::insert (int idx, const QString& text)
{
    _text.insert(idx, text);
}

void Document::remove (int idx, int length)
{
    _text.remove(idx, length);
}

TextField::TextField (int minWidth, Document* document, QObject* parent) :
    Component(parent),
    _minWidth(minWidth),
    _document(document),
    _documentPos(0),
    _cursorPos(document->text().length())
{
}

TextField::TextField (int minWidth, const QString& text, QObject* parent) :
    Component(parent),
    _minWidth(minWidth),
    _document(new Document(text)),
    _documentPos(0),
    _cursorPos(text.length())
{
}

TextField::~TextField ()
{
    delete _document;
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
    QRect inner = innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width();

    ctx->drawChar(x, y, '[');

    // draw the visible portion of the contents
    const QString& text = _document->text();
    int length = text.length();

    // get the document position from the front
    ctx->drawString(x + 1, y, text.constData() + _documentPos,
        qMin(length - _documentPos, width - 2));

    // draw the cursor if in focus
    if (_focused) {
        ctx->drawChar(x + 1 + (_cursorPos - _documentPos), y,
            REVERSE_FLAG | (_cursorPos < length ? text.at(_cursorPos).unicode() : ' '));
    }

    ctx->drawChar(x + width - 1, y, ']');
}

void TextField::focusInEvent (QFocusEvent* e)
{
    dirty();
}

void TextField::focusOutEvent (QFocusEvent* e)
{
    dirty();
}

void TextField::mouseButtonPressEvent (QMouseEvent* e)
{
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
            updateDocumentPos();
        }
        return;
    }
    int key = e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers == Qt::ControlModifier) {
        if (key == Qt::Key_Left) { // previous word beginning
            const QString& text = _document->text();
            if (_cursorPos > 0) {
                for (int idx = _cursorPos - 1; idx > 0; idx--) {
                    if (text.at(idx).isLetterOrNumber() && !text.at(idx - 1).isLetterOrNumber()) {
                        _cursorPos = idx;
                        updateDocumentPos();
                        return;
                    }
                }
                _cursorPos = 0;
                updateDocumentPos();
            }
        } else if (key == Qt::Key_Right) { // next word end
            const QString& text = _document->text();
            if (_cursorPos < text.length()) {
                for (int idx = _cursorPos + 1, max = text.length(); idx < max; idx++) {
                    if (!text.at(idx).isLetterOrNumber() && text.at(idx - 1).isLetterOrNumber()) {
                        _cursorPos = idx;
                        updateDocumentPos();
                        return;
                    }
                }
                _cursorPos = text.length();
                updateDocumentPos();
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
                updateDocumentPos();
            }
            break;
        case Qt::Key_Right:
            if (_cursorPos < _document->text().length()) {
                _cursorPos++;
                updateDocumentPos();
            }
            break;
        case Qt::Key_Home:
            if (_cursorPos > 0) {
                _cursorPos = 0;
                updateDocumentPos();
            }
            break;
        case Qt::Key_End:
            if (_cursorPos < _document->text().length()) {
                _cursorPos = _document->text().length();
                updateDocumentPos();
            }
            break;
        case Qt::Key_Backspace:
            if (_cursorPos > 0) {
                _document->remove(--_cursorPos, 1);
                updateDocumentPos();
            }
            break;
        case Qt::Key_Delete:
            if (_cursorPos < _document->text().length()) {
                _document->remove(_cursorPos, 1);
                dirty();
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

void TextField::updateDocumentPos ()
{
    int width = innerRect().width() - 3;
    _documentPos = qBound(qMax(_cursorPos - width, 0), _documentPos,
        qMin(_cursorPos, _document->text().length() - width));
    dirty();
}
