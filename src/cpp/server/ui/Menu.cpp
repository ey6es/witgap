//
// $Id$

#include <QKeyEvent>
#include <QKeySequence>

#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Layout.h"
#include "ui/Menu.h"
#include "util/Callback.h"

Menu::Menu (Session* parent, int deleteOnReleaseKey) :
    Window(parent, parent->highestWindowLayer(), true, true),
    _deleteOnReleaseKey(deleteOnReleaseKey)
{
    setBorder(new FrameBorder());
    setLayout(new TableLayout(1));

    if (deleteOnReleaseKey != -1) {
        setFocus(this);
    }
}

Button* Menu::addButton (const QString& label, const QMetaObject* metaObject,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9)
{
    Creator* creator = new Creator(
        0, metaObject, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
    Button* button = addButton(label, creator, SLOT(create()));
    creator->setParent(button);
    return button;
}

Button* Menu::addButton (const QString& label, QObject* obj, const char* method)
{
    Button* button = new Button(label);
    addChild(button);
    obj->connect(button, SIGNAL(pressed()), method);
    connect(button, SIGNAL(pressed()), SLOT(deleteLater()));
    return button;
}

void Menu::keyPressEvent (QKeyEvent* e)
{
    // search for a button mnemonic
    QString str = QKeySequence(e->key()).toString();
    if (str.length() != 1) {
        Window::keyPressEvent(e);
        return;
    }
    QChar upper = str.at(0), lower = upper.toLower();
    foreach (Button* button, findChildren<Button*>()) {
        const QString& label = button->label();
        for (const QChar* ptr = label.constData(), *end = ptr + label.length(); ptr < end; ptr++) {
            QChar ch = *ptr;
            if (ch == '&' && ++ptr < end && (*ptr == upper || *ptr == lower)) {
                button->doPress();
                return;
            }
        }
    }
    Window::keyPressEvent(e);
}

void Menu::keyReleaseEvent (QKeyEvent* e)
{
    if (e->key() == _deleteOnReleaseKey) {
        deleteLater();

    } else {
        Window::keyReleaseEvent(e);
    }
}
