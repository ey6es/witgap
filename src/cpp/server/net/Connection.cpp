//
// $Id$

#include <iostream>

#include <QMetaMethod>
#include <QPoint>
#include <QRect>
#include <QUrl>
#include <QtDebug>
#include <QtEndian>

#include <openssl/rsa.h>

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

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->connectionManager()),
    _app(app),
    _socket(socket),
    _stream(socket)
{
    // init crypto bits
    EVP_CIPHER_CTX_init(&_ectx);
    EVP_CIPHER_CTX_init(&_dctx);

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

    // cleanup crypto bits
    EVP_CIPHER_CTX_cleanup(&_ectx);
    EVP_CIPHER_CTX_cleanup(&_dctx);
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
    // set in our local map
    _cookies.insert(name, value);

    QByteArray nbytes = name.toUtf8(), vbytes = value.toUtf8();
    _stream << (quint16)(3 + nbytes.length() + vbytes.length());
    _stream << SET_COOKIE_MSG;
    _stream << (quint16)nbytes.length();
    _socket->write(nbytes);
    _socket->write(vbytes);
}

/**
 * Helper function for readHeader: puts an int back in the socket buffer.
 */
static void ungetInt (QTcpSocket* socket, quint32 value)
{
    socket->ungetChar(value & 0xFF);
    socket->ungetChar((value >> 8) & 0xFF);
    socket->ungetChar((value >> 16) & 0xFF);
    socket->ungetChar(value >> 24);
}

void Connection::readHeader ()
{
    // if we don't have the full header, wait until we do
    if (_socket->bytesAvailable() < 12) {
        return;
    }
    // check the magic number and version
    quint32 magic, version, length;
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
    _stream >> length;
    if (_socket->bytesAvailable() < length) {
        // push back what we read and wait for the rest
        ungetInt(_socket, length);
        ungetInt(_socket, PROTOCOL_VERSION);
        ungetInt(_socket, PROTOCOL_MAGIC);
        return;
    }
    quint16 width, height;
    _stream >> width;
    _stream >> height;
    _displaySize = QSize(width, height);

    QByteArray encryptedSecret = _socket->read(128);
    QByteArray secret(32, 0);
    int len = RSA_private_decrypt(128, (const unsigned char*)encryptedSecret.constData(),
        (unsigned char*)secret.data(), _app->connectionManager()->rsa(), RSA_PKCS1_PADDING);
    if (len != 32) {
        qWarning() << "Invalid encryption key:" << _socket->peerAddress();
        deactivate("Invalid encryption key.");
        return;
    }
    EVP_EncryptInit_ex(&_ectx, EVP_aes_128_cbc(), 0, (unsigned char*)secret.data(),
        (unsigned char*)secret.data() + 16);
    EVP_DecryptInit_ex(&_dctx, EVP_aes_128_cbc(), 0, (unsigned char*)secret.data(),
        (unsigned char*)secret.data() + 16);

    // the rest is encrypted with the secret key
    QByteArray in = _socket->read(length - 4 - 128);
    QByteArray out(in.length(), 0);
    int outl;
    EVP_DecryptUpdate(&_dctx, (unsigned char*)out.data(), &outl,
        (unsigned char*)in.data(), in.length());
    EVP_DecryptFinal_ex(&_dctx, (unsigned char*)out.data() + outl, &outl);
    QDataStream ostream(out);
    quint16 qlen, clen;
    ostream >> qlen;
    QByteArray query(qlen, 0);
    ostream.readRawData(query.data(), qlen);
    ostream >> clen;
    QByteArray cookie(clen, 0);
    ostream.readRawData(cookie.data(), clen);

    // parse out the query and cookie values
    if (query.startsWith('?')) {
        query.remove(0, 1);
    }
    foreach (const QByteArray& str, query.split('&')) {
        QByteArray tstr = str.trimmed();
        int idx = tstr.indexOf('=');
        _query.insert((idx == -1) ? "" : QUrl::fromPercentEncoding(tstr.left(idx)),
            QUrl::fromPercentEncoding(tstr.mid(idx + 1)));
    }
    foreach (const QByteArray& str, cookie.split(';')) {
        QByteArray tstr = str.trimmed();
        int idx = tstr.indexOf('=');
        _cookies.insert((idx == -1) ? "" : QUrl::fromPercentEncoding(tstr.left(idx)),
            QUrl::fromPercentEncoding(tstr.mid(idx + 1)));
    }

    // disable the connection until we're ready to read subsequent messages
    _socket->disconnect(this);

    // log the establishment
    qDebug() << "Connection established." << _displaySize;

    // notify the connection manager, which will create a session for the connection
    _app->connectionManager()->connectionEstablished(this);
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
