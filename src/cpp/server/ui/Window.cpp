//
// $Id$

#include "net/Connection.h"
#include "net/Session.h"
#include "ui/Window.h"

Window::Window (QObject* parent, int layer) :
    Container(parent),
    _id(session()->nextWindowId()),
    _layer(layer),
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
}

Window::~Window ()
{
    // if the session still exists and is connected, send a remove window message
    Session* sess = session();
    if (sess != 0) {
        Connection* connection = sess->connection();
        if (connection != 0) {
            Connection::removeWindowMetaMethod().invoke(connection, Q_ARG(int, _id));
        }
    }
}

Session* Window::session () const
{
    for (QObject* obj = parent(); obj != 0; obj = obj->parent()) {
        Session* session = qobject_cast<Session*>(obj);
        if (session != 0) {
            return session;
        }
    }
    return 0;
}

void Window::setLayer (int layer)
{
    if (_layer != layer) {
        _layer = layer;
        noteNeedsUpdate();
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

void Window::sync ()
{
    Connection* connection = session()->connection();
    if (connection != 0) {
        if (!_upToDate) {
            const QMetaMethod& method = _added ?
                Connection::updateWindowMetaMethod() : Connection::addWindowMetaMethod();
            method.invoke(connection, Q_ARG(int, _id), Q_ARG(int, _layer),
                Q_ARG(const QRect&, _bounds), Q_ARG(int, _background));
            _upToDate = _added = true;
        }

        maybeValidate();

        if (!_dirty.isEmpty()) {
            // draw the affected area
            prepareForDrawing();
            draw(this);

            // update the affected regions
            const QMetaMethod& method = Connection::setContentsMetaMethod();
            for (int ii, nn = _rects.size(); ii < nn; ii++) {
                method.invoke(connection, Q_ARG(int, _id), Q_ARG(const QRect&, _rects.at(ii)),
                    Q_ARG(const QIntVector&, _buffers.at(ii)));
            }

            // clear the dirty region
            _dirty = QRegion();
        }
    }
    _syncEnqueued = false;
}

const QMetaMethod& Window::syncMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("sync()"));
    return method;
}
