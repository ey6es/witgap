//
// $Id$

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
#include "ui/Window.h"

Session::Session (ServerApp* app, Connection* connection, quint64 id, const QByteArray& token) :
    QObject(app->connectionManager()),
    _app(app),
    _connection(0),
    _id(id),
    _token(token),
    _lastWindowId(0),
    _focused(0)
{
    // send the session info back to the connection and activate it
    connection->setSession(id, token);
    setConnection(connection);

    showConfirmDialog(tr("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis vel nunc "
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
    connect(_connection, SIGNAL(keyPressed(int)), SLOT(dispatchKeyPressed(int)));
    connect(_connection, SIGNAL(keyReleased(int)), SLOT(dispatchKeyReleased(int)));
    _connection->activate();

    // readd the windows
    foreach (Window* window, findChildren<Window*>()) {
        window->resend();
    }
}

void Session::setFocus (Component* component)
{
    if (_focused == component) {
        return;
    }
    if (_focused != 0) {
        _focused->disconnect(this);
        QFocusEvent event(QEvent::FocusOut);
        QCoreApplication::sendEvent(_focused, &event);
    }
    if ((_focused = component) != 0) {
        connect(_focused, SIGNAL(destroyed()), SLOT(clearFocus()));
        QFocusEvent event(QEvent::FocusIn);
        QCoreApplication::sendEvent(_focused, &event);
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

void Session::clearConnection ()
{
    _connection = 0;
}

void Session::clearFocus ()
{
    _focused = 0;
}

void Session::dispatchMousePressed (int x, int y)
{
    QMouseEvent event(QEvent::MouseButtonPress, QPoint(x, y), QPoint(x, y),
        Qt::LeftButton, Qt::LeftButton, _modifiers);
    QCoreApplication::sendEvent(this, &event);
}

void Session::dispatchMouseReleased (int x, int y)
{
    QMouseEvent event(QEvent::MouseButtonRelease, QPoint(x, y), QPoint(x, y),
        Qt::LeftButton, Qt::NoButton, _modifiers);
    QCoreApplication::sendEvent(this, &event);
}

void Session::dispatchKeyPressed (int key)
{
    QKeyEvent event(QEvent::KeyPress, key, _modifiers);
    QCoreApplication::sendEvent(_focused == 0 ? this : (QObject*)_focused, &event);
}

void Session::dispatchKeyReleased (int key)
{
    QKeyEvent event(QEvent::KeyRelease, key, _modifiers);
    QCoreApplication::sendEvent(_focused == 0 ? this : (QObject*)_focused, &event);
}
