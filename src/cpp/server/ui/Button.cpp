//
// $Id$

#include "Protocol.h"
#include "ui/Button.h"

Button::Button (const QIntVector& text, Qt::Alignment alignment, QObject* parent) :
    Label(text, alignment, parent)
{
    // give the button some extra space
    _text.prepend(' ');
    _text.append(' ');
}

void Button::invalidate ()
{
    Label::invalidate();
    setTextFlags(_focused ? REVERSE_FLAG : 0);
}

void Button::focusInEvent (QFocusEvent* e)
{
    setTextFlags(REVERSE_FLAG);
}

void Button::focusOutEvent (QFocusEvent* e)
{
    setTextFlags(0);
}

void Button::keyPressEvent (QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return) {
        emit pressed();

    } else {
        Label::keyPressEvent(e);
    }
}
