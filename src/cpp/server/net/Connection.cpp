//
// $Id$

#include <iostream>

#include <QMetaMethod>
#include <QPoint>
#include <QRect>
#include <QtDebug>
#include <QtEndian>

#include "Protocol.h"
#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "util/General.h"

const QMetaMethod& Connection::updateWindowMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("updateWindow(int,int,QRect,int)"));
    return method;
}

const QMetaMethod& Connection::removeWindowMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("removeWindow(int)"));
    return method;
}

const QMetaMethod& Connection::setContentsMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("setContents(int,QRect,QIntVector)"));
    return method;
}

const QMetaMethod& Connection::setCookieMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("setCookie(QString,QString)"));
    return method;
}

const QMetaMethod& Connection::requestCookieMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("requestCookie(QString,Callback)"));
    return method;
}

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->connectionManager()),
    _app(app),
    _socket(socket),
    _stream(socket),
    _lastCookieRequestId(0)
{
    // take over ownership of the socket
    _socket->setParent(this);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readHeader()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));
    connect(this, SIGNAL(windowClosed()), SLOT(deleteLater()));

    // log the connection
    qDebug() << "Connection opened." << (_address = _socket->peerAddress());
}

Connection::~Connection ()
{
    // log the destruction
    QString error;
    QDebug base = qDebug() << "Connection closed." << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
}

void Connection::activate ()
{
    // connect the signal in order to read incoming messages
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));

    // call it manually in case we already have something to read
    readMessages();
}

void Connection::deactivate (const QString& reason)
{
    QByteArray rbytes = reason.toUtf8();
    _stream << (quint16)(1 + rbytes.length());
    _stream << CLOSE_MSG;
    _socket->write(rbytes);
    _socket->disconnectFromHost();
}

void Connection::updateWindow (int id, int layer, const QRect& bounds, int fill)
{
    _stream << (qint16)21;
    _stream << UPDATE_WINDOW_MSG;
    _stream << (qint32)id;
    _stream << (qint32)layer;
    write(bounds);
    _stream << (qint32)fill;
}

void Connection::removeWindow (int id)
{
    _stream << (qint16)5;
    _stream << REMOVE_WINDOW_MSG;
    _stream << (qint32)id;
}

void Connection::setContents (int id, const QRect& bounds, const QIntVector& contents)
{
    int size = contents.size();

    _stream << (quint16)(13 + size*4);
    _stream << SET_CONTENTS_MSG;
    _stream << (qint32)id;
    write(bounds);
    for (int ii = 0; ii < size; ii++) {
        _stream << (qint32)contents.at(ii);
    }
}

void Connection::moveContents (int id, const QRect& source, const QPoint& dest, int fill)
{
    _stream << (quint16)21;
    _stream << MOVE_CONTENTS_MSG;
    _stream << (qint32)id;
    write(source);
    _stream << (qint16)dest.x();
    _stream << (qint16)dest.y();
    _stream << (qint32)fill;
}

void Connection::setCookie (const QString& name, const QString& value)
{
    QByteArray nbytes = name.toUtf8(), vbytes = value.toUtf8();

    _stream << (quint16)(3 + nbytes.length() + vbytes.length());
    _stream << SET_COOKIE_MSG;
    _stream << (quint16)nbytes.length();
    _socket->write(nbytes);
    _socket->write(vbytes);
}

void Connection::requestCookie (const QString& name, const Callback& callback)
{
    QByteArray nbytes = name.toUtf8();

    _stream << (quint16)(5 + nbytes.length());
    _stream << REQUEST_COOKIE_MSG;
    _stream << (quint32)(++_lastCookieRequestId);
    _socket->write(nbytes);

    _cookieRequests.insert(_lastCookieRequestId, callback);
}

void Connection::readHeader ()
{
    // if we don't have the full header, wait until we do
    if (_socket->bytesAvailable() < 36) {
        return;
    }
    // check the magic number and version
    quint32 magic, version;
    _stream >> magic;
    if (magic != PROTOCOL_MAGIC) {
        qWarning() << "Invalid protocol magic number:" << magic << _socket->peerAddress();
        deactivate("Invalid magic number.");
        return;
    }
    _stream >> version;
    if (version != PROTOCOL_VERSION) {
        qWarning() << "Wrong protocol version:" << version << _socket->peerAddress();
        deactivate("Wrong protocol version.");
        return;
    }
    quint64 sessionId;
    _stream >> sessionId;
    QByteArray sessionToken = _socket->read(16);
    quint16 width, height;
    _stream >> width;
    _stream >> height;
    _displaySize = QSize(width, height);

    // disable the connection until we're ready to read subsequent messages
    _socket->disconnect(this);

    // log the establishment
    qDebug() << "Connection established." << sessionId << _displaySize;

    // notify the connection manager, which will create a session for the connection
    _app->connectionManager()->connectionEstablished(this, sessionId, sessionToken);
}

void Connection::readMessages ()
{
    // read as many messages as are available
    while (true) {
        qint64 available = _socket->bytesAvailable();
        if (available < 1) {
            return; // wait until we have the rest of the header
        }
        char type;
        _socket->getChar(&type);
        if (type == WINDOW_CLOSED_MSG) {
            emit windowClosed();

        } else if (type == REPORT_COOKIE_MSG) {
            if (available < 3) {
                _socket->ungetChar(type);
                return; // wait until we have the length
            }
            quint16 length;
            _stream >> length;
            if (available < 3 + length) {
                _socket->ungetChar(length & 0xFF);
                _socket->ungetChar(length >> 8);
                _socket->ungetChar(type);
                return; // wait until we have the data
            }
            quint32 id;
            _stream >> id;
            QByteArray value = _socket->read(length - 4);
            _cookieRequests.take(id).invoke(Q_ARG(const QString&, value));

        } else if (type == MOUSE_PRESSED_MSG || type == MOUSE_RELEASED_MSG) {
            if (available < 5) {
                _socket->ungetChar(type);
                return; // wait until we have the data
            }
            quint16 x, y;
            _stream >> x;
            _stream >> y;
            emit (type == MOUSE_PRESSED_MSG) ? mousePressed(x, y) : mouseReleased(x, y);

        } else {
            // type == KEY_PRESSED_MSG || type == KEY_PRESSED_NUMPAD_MSG ||
            // type == KEY_RELEASED_MSG || type == KEY_RELEASED_NUMPAD_MSG
            if (available < 7) {
                _socket->ungetChar(type);
                return; // wait until we have the data
            }
            quint32 key;
            quint16 ch;
            _stream >> key;
            _stream >> ch;
            switch (type) {
                case KEY_PRESSED_MSG:
                    emit keyPressed(key, ch, false);
                    break;
                case KEY_PRESSED_NUMPAD_MSG:
                    emit keyPressed(key, ch, true);
                    break;
                case KEY_RELEASED_MSG:
                    emit keyReleased(key, ch, false);
                    break;
                case KEY_RELEASED_NUMPAD_MSG:
                    emit keyReleased(key, ch, true);
                    break;
            }
        }
    }
}

void Connection::write (const QRect& rect)
{
    _stream << (qint16)rect.left();
    _stream << (qint16)rect.top();
    _stream << (qint16)rect.width();
    _stream << (qint16)rect.height();
}
