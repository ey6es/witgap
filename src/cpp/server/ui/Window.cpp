//
// $Id$

#include <QCoreApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QtDebug>

#include "net/Connection.h"
#include "net/Session.h"
#include "ui/Window.h"

Window::Window (QObject* parent, int layer, bool modal, bool deleteOnEscape) :
    Container(0, parent),
    _id(session()->nextWindowId()),
    _layer(layer),
    _modal(modal),
    _deleteOnEscape(deleteOnEscape),
    _focus(0),
    _active(false),
    _syncEnqueued(true),
    _added(false),
    _upToDate(false)
{
    // use an opaque background
    _background = ' ';

    // connect slots
    connect(this, SIGNAL(boundsChanged()), SLOT(noteNeedsUpdate()));

    // queue up a sync message
    syncMetaMethod().invoke(this, Qt::QueuedConnection);

    // if modal, we need to update the active window first thing
    if (modal) {
        session()->updateActiveWindow();
    }
}

Window::~Window ()
{
    // if the session still exists and is connected, send a remove window message
    Session* session = this->session();
    if (session != 0 && _added) {
        Connection* connection = session->connection();
        if (connection != 0) {
            Connection::removeWindowMetaMethod().invoke(connection, Q_ARG(int, _id));
        }
    }
}

void Window::setLayer (int layer)
{
    if (_layer != layer) {
        _layer = layer;
        noteNeedsUpdate();
        session()->updateActiveWindow();
    }
}

void Window::setModal (bool modal)
{
    if (_modal != modal) {
        _modal = modal;
        session()->updateActiveWindow();
    }
}

void Window::setFocus (Component* focus)
{
    if (_focus == focus) {
        return;
    }
    bool ozero = (_focus == 0);
    if (!ozero) {
        _focus->disconnect(this, SLOT(clearFocus()));
        if (_active) {
            QFocusEvent event(QEvent::FocusOut);
            QCoreApplication::sendEvent(_focus, &event);
        }
    }
    bool nzero = ((_focus = focus) == 0);
    if (!nzero) {
        connect(focus, SIGNAL(destroyed()), SLOT(clearFocus()));
        if (_active) {
            QFocusEvent event(QEvent::FocusIn);
            QCoreApplication::sendEvent(focus, &event);
        }
    }
    if (ozero != nzero) {
        session()->updateActiveWindow();
    }
}

void Window::setActive (bool active)
{
    if (_active == active) {
        return;
    }
    _active = active;
    if (_focus != 0) {
        QFocusEvent event(_active ? QEvent::FocusIn : QEvent::FocusOut);
        QCoreApplication::sendEvent(_focus, &event);
    }
}

void Window::pack ()
{
    setBounds(QRect(_bounds.topLeft(), preferredSize(-1, -1)));
}

void Window::center ()
{
    QSize dsize = session()->displaySize();
    QSize wsize = _bounds.size();
    setBounds(QRect(QPoint((dsize.width() - wsize.width())/2,
        (dsize.height() - wsize.height())/2), wsize));
}

void Window::maybeResend ()
{
    _added = false;

    if (_visible) {
        noteNeedsUpdate();
        Component::dirty();
    }
}

void Window::setVisible (bool visible)
{
    if (_visible != visible) {
        _visible = visible;
        maybeEnqueueSync();
        session()->updateActiveWindow();

        if (visible) {
            Component::dirty();
        }
    }
}

void Window::invalidate ()
{
    _valid = false;
    maybeEnqueueSync();
}

void Window::dirty (const QRect& region)
{
    _dirty += region;
    maybeEnqueueSync();
}

void Window::scrollDirty (const QRect& region, const QPoint& delta)
{
    QRect overlap = region.intersected(region.translated(delta));
    QRegion moved = _dirty.intersected(overlap.translated(-delta.x(), -delta.y()));
    moved.translate(delta);
    _dirty += region;
    _dirty -= QRegion(overlap);
    _dirty += moved;

    maybeEnqueueSync();
}

void Window::noteNeedsUpdate ()
{
    _upToDate = false;
    maybeEnqueueSync();
}

void Window::maybeEnqueueSync ()
{
    if (!_syncEnqueued) {
        syncMetaMethod().invoke(this, Qt::QueuedConnection);
        _syncEnqueued = true;
    }
}

void Window::clearFocus ()
{
    _focus = 0;

    // try to reassign
    transferFocus(0, Forward);
}

void Window::validate ()
{
    Container::validate();

    // make sure the focus is still valid
    if (_focus == 0) {
        transferFocus(0, Forward);
    } else if (!(_focus->acceptsFocus() || _focus->transferFocus(_focus, Forward))) {
        setFocus(0);
    }
}

void Window::draw (DrawContext* ctx)
{
    // if the background is zero, we must force a fill (else Component::draw will take care of it)
    if (_background == 0) {
        ctx->fillRect(0, 0, _bounds.width(), _bounds.height(), 0, true);
    }
    Container::draw(ctx);
}

void Window::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (_deleteOnEscape && e->key() == Qt::Key_Escape &&
            (modifiers == Qt::ShiftModifier || modifiers == Qt::NoModifier)) {
        deleteLater();
    } else {
        Container::keyPressEvent(e);
    }
}

void Window::moveContents (const QRect& source, const QPoint& dest)
{
    Connection::moveContentsMetaMethod().invoke(session()->connection(), Q_ARG(int, _id),
        Q_ARG(const QRect&, source), Q_ARG(const QPoint&, dest), Q_ARG(int, _background));
}

void Window::sync ()
{
    // make sure we have a connection
    Connection* connection = session()->connection();
    if (connection == 0) {
        _syncEnqueued = false;
        return;
    }

    // compound all messages in the sync
    Connection::Compounder compound(connection);

    // remove window if hidden
    if (!_visible) {
        if (_added) {
            Connection::removeWindowMetaMethod().invoke(connection, Q_ARG(int, _id));
            _added = false;
        }
        _syncEnqueued = false;
        return;
    }

    // add or update window if necessary
    if (!(_added && _upToDate)) {
        Connection::updateWindowMetaMethod().invoke(connection, Q_ARG(int, _id),
            Q_ARG(int, _layer), Q_ARG(const QRect&, _bounds), Q_ARG(int, _background));
        _added = true;
        _upToDate = true;
    }

    maybeValidate();

    // clip the dirty region to the window bounds
    _dirty &= QRect(0, 0, _bounds.width(), _bounds.height());
    if (!_dirty.isEmpty()) {
        prepareForDrawing();
        draw(this);

        // send the contents of the affected regions
        const QMetaMethod& method = Connection::setContentsMetaMethod();
        for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
            method.invoke(connection, Q_ARG(int, _id), Q_ARG(const QRect&, _rects.at(ii)),
                Q_ARG(const QIntVector&, _buffers.at(ii)));
        }

        // clear the dirty region
        _dirty = QRegion();
    }
    _syncEnqueued = false;
}

const QMetaMethod& Window::syncMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("sync()"));
    return method;
}

EncryptedWindow::EncryptedWindow (QObject* parent, int layer, bool modal, bool deleteOnEscape) :
    Window(parent, layer, modal, deleteOnEscape)
{
    session()->incrementCryptoCount();
}

EncryptedWindow::~EncryptedWindow ()
{
    Session* session = this->session();
    if (session != 0) {
        session->decrementCryptoCount();
    }
}
