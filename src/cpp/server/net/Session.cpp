//
// $Id$

#include <limits>

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QtDebug>

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Component.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"
#include "ui/Window.h"

using namespace std;

Session::Session (ServerApp* app, Connection* connection, quint64 id, const QByteArray& token) :
    QObject(app->connectionManager()),
    _app(app),
    _connection(0),
    _id(id),
    _token(token),
    _lastWindowId(0),
    _moused(0),
    _focus(0)
{
    // send the session info back to the connection and activate it
    connection->setSession(id, token);
    setConnection(connection);

    showInputDialog(tr("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis vel nunc "
        "justo. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos "
        "himenaeos. Praesent ut elit ipsum, sit amet dictum est. Cras purus augue, cursus sed "
        "elementum eu, tincidunt ac purus. Vivamus id lorem felis, sed posuere odio. Aenean "
        "varius placerat euismod. Curabitur vel luctus odio. Vivamus sagittis sapien in eros "
        "malesuada suscipit. Sed lacus massa, vulputate id dictum non, interdum ut odio. "
        "Vestibulum luctus sollicitudin ligula, vitae elementum dui dictum vitae. Nullam in leo "
        "justo. Praesent sem mauris, porttitor a vehicula ut, aliquam id arcu. Ut varius interdum "
        "sagittis."), Callback());
}

void Session::setConnection (Connection* connection)
{
    if (_connection != 0) {
        _connection->disconnect(this);
        _connection->deactivate(tr("Logged in elsewhere."));
    }
    _connection = connection;
    _displaySize = connection->displaySize();
    connect(_connection, SIGNAL(windowClosed()), SLOT(deleteLater()));
    connect(_connection, SIGNAL(destroyed()), SLOT(clearConnection()));
    connect(_connection, SIGNAL(mousePressed(int,int)), SLOT(dispatchMousePressed(int,int)));
    connect(_connection, SIGNAL(mouseReleased(int,int)), SLOT(dispatchMouseReleased(int,int)));
    connect(_connection, SIGNAL(keyPressed(int,QChar)), SLOT(dispatchKeyPressed(int,QChar)));
    connect(_connection, SIGNAL(keyReleased(int,QChar)), SLOT(dispatchKeyReleased(int,QChar)));
    _connection->activate();

    // clear the modifiers
    _modifiers = Qt::KeyboardModifiers();

    // readd the windows
    foreach (Window* window, findChildren<Window*>()) {
        window->resend();
    }
}

void Session::setFocus (Component* component)
{
    if (_focus == component) {
        return;
    }
    if (_focus != 0) {
        _focus->disconnect(this, SLOT(clearFocus()));
        QFocusEvent event(QEvent::FocusOut);
        QCoreApplication::sendEvent(_focus, &event);
    }
    if ((_focus = component) != 0) {
        connect(_focus, SIGNAL(destroyed()), SLOT(clearFocus()));
        QFocusEvent event(QEvent::FocusIn);
        QCoreApplication::sendEvent(_focus, &event);
    }
}

/**
 * Helper for the various dialog functions.
 */
static Window* createDialog (Session* session, const QString& message, const QString& title)
{
    Window* window = new Window(session);
    window->setModal(true);
    window->setBorder(title.isEmpty() ? new FrameBorder() : new TitledBorder(title));
    window->setLayout(new BoxLayout(Qt::Vertical));
    Label* label = new Label(message, Qt::AlignCenter);
    label->setPreferredSize(QSize(40, -1));
    window->addChild(label);
    return window;
}

void Session::showInfoDialog (const QString& message, const QString& title, const QString& dismiss)
{
    Window* window = createDialog(this, message, title);

    Button* button = new Button(dismiss.isEmpty() ? tr("OK") : dismiss);
    window->addChild(button);
    window->connect(button, SIGNAL(pressed()), SLOT(deleteLater()));
    button->requestFocus();
    window->pack();
    window->center();
}

void Session::showConfirmDialog (
    const QString& message, const Callback& callback, const QString& title,
    const QString& dismiss, const QString& accept)
{
    Window* window = createDialog(this, message, title);

    Container* buttons = new Container(new BoxLayout());
    window->addChild(buttons);

    Button* cancel = new Button(dismiss.isEmpty() ? tr("Cancel") : dismiss);
    buttons->addChild(cancel);
    window->connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));

    Button* ok = new Button(accept.isEmpty() ? tr("OK") : accept);
    buttons->addChild(ok);
    window->connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    (new CallbackObject(callback, window))->connect(ok, SIGNAL(pressed()), SLOT(invoke()));

    ok->requestFocus();
    window->pack();
    window->center();
}

void Session::showInputDialog (
    const QString& message, const Callback& callback, const QString& title,
    const QString& dismiss, const QString& accept)
{
    Window* window = createDialog(this, message, title);

    TextField* field = new TextField(30, tr("Lorem ipsum dolor sit amet, consectetur adipiscing elit."));
    window->addChild(field);

    Container* buttons = new Container(new BoxLayout());
    window->addChild(buttons);

    Button* cancel = new Button(dismiss.isEmpty() ? tr("Cancel") : dismiss);
    buttons->addChild(cancel);
    window->connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));

    Button* ok = new Button(accept.isEmpty() ? tr("OK") : accept);
    buttons->addChild(ok);
    window->connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    (new CallbackObject(callback, window))->connect(ok, SIGNAL(pressed()), SLOT(invoke()));

    field->requestFocus();
    window->pack();
    window->center();
}

void Session::clearConnection ()
{
    _connection = 0;
}

void Session::clearMoused ()
{
    _moused = 0;
}

void Session::clearFocus ()
{
    _focus = 0;
}

void Session::dispatchMousePressed (int x, int y)
{
    // we shouldn't have a moused component, but let's make sure
    if (_moused != 0) {
        _moused->disconnect(this, SLOT(clearMoused()));
        _moused = 0;
    }

    // find the highest "hit"
    int hlayer = numeric_limits<int>::min();
    Component* target = 0;
    QPoint absolute(x, y), relative(x, y);
    foreach (Window* window, findChildren<Window*>()) {
        if (window->layer() < hlayer) {
            continue;
        }
        if (window->bounds().contains(absolute)) {
            QPoint rel;
            Component* comp = window->componentAt(absolute - window->bounds().topLeft(), &rel);
            if (comp != 0) {
                target = comp;
                relative = rel;
            }
        }
    }
    if (target != 0) {
        connect(_moused = target, SIGNAL(destroyed()), SLOT(clearMoused()));
    }
    QMouseEvent event(QEvent::MouseButtonPress, relative, absolute,
        Qt::LeftButton, Qt::LeftButton, _modifiers);
    QCoreApplication::sendEvent(target == 0 ? this : (QObject*)target, &event);
}

void Session::dispatchMouseReleased (int x, int y)
{
    QPoint absolute(x, y), relative(x, y);
    Component* target = _moused;
    if (_moused != 0) {
        relative -= _moused->absolutePos();
        _moused->disconnect(this, SLOT(clearMoused()));
        _moused = 0;
    }
    QMouseEvent event(QEvent::MouseButtonRelease, relative, absolute,
        Qt::LeftButton, Qt::NoButton, _modifiers);
    QCoreApplication::sendEvent(target == 0 ? this : (QObject*)target, &event);
}

/**
 * Helper function for modifier updates.
 */
Qt::KeyboardModifier getModifier (int key)
{
    switch (key) {
        case Qt::Key_Shift:
            return Qt::ShiftModifier;
        case Qt::Key_Control:
            return Qt::ControlModifier;
        case Qt::Key_Alt:
            return Qt::AltModifier;
        case Qt::Key_Meta:
            return Qt::MetaModifier;
        default:
            return Qt::NoModifier;
    }
}

void Session::dispatchKeyPressed (int key, QChar ch)
{
    // update our modifiers
    Qt::KeyboardModifier modifier = getModifier(key);
    if (modifier != Qt::NoModifier) {
        _modifiers |= modifier;
    }
    QKeyEvent event(QEvent::KeyPress, key, _modifiers, ch == 0 ? QString() : QString(ch));
    QCoreApplication::sendEvent(_focus == 0 ? this : (QObject*)_focus, &event);
}

void Session::dispatchKeyReleased (int key, QChar ch)
{
    // update our modifiers
    Qt::KeyboardModifier modifier = getModifier(key);
    if (modifier != Qt::NoModifier) {
        _modifiers &= ~modifier;
    }
    QKeyEvent event(QEvent::KeyRelease, key, _modifiers, ch == 0 ? QString() : QString(ch));
    QCoreApplication::sendEvent(_focus == 0 ? this : (QObject*)_focus, &event);
}
