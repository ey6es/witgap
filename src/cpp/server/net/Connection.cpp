//
// $Id$

#include <iostream>

#include <QBuffer>
#include <QDateTime>
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

// register our types with the metatype system
int sharedConnectionPointerType = qRegisterMetaType<SharedConnectionPointer>(
    "SharedConnectionPointer");

Connection::Compounder::Compounder (Connection* connection) :
    _connection(connection)
{
    startCompoundMetaMethod().invoke(connection);
}

Connection::Compounder::~Compounder ()
{
    commitCompoundMetaMethod().invoke(_connection);
}

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

const QMetaMethod& Connection::moveContentsMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("moveContents(int,QRect,QPoint,int)"));
    return method;
}

const QMetaMethod& Connection::setCookieMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("setCookie(QString,QString)"));
    return method;
}

const QMetaMethod& Connection::toggleCryptoMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("toggleCrypto()"));
    return method;
}

const QMetaMethod& Connection::startCompoundMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("startCompound()"));
    return method;
}

const QMetaMethod& Connection::commitCompoundMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("commitCompound()"));
    return method;
}

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    _pointer(this, &QObject::deleteLater),
    _app(app),
    _socket(socket),
    _stream(socket),
    _crypto(false),
    _clientCrypto(false),
    _compoundCount(0),
    _throttle(50, 1000)
{
    // init crypto bits
    EVP_CIPHER_CTX_init(&_ectx);
    EVP_CIPHER_CTX_init(&_dctx);

    // take over ownership of the socket and set its low delay option
    _socket->setParent(this);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readHeader()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(close()));
    connect(socket, SIGNAL(disconnected()), SLOT(close()));
    connect(this, SIGNAL(windowClosed()), SLOT(close()));

    // log the connection
    qDebug() << "Connection opened." << (_address = _socket->peerAddress());
}

Connection::~Connection ()
{
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
    int rlen = rbytes.length();
    startMessage(1 + rlen);
    _stream << CLOSE_MSG;
    _stream.writeRawData(rbytes.constData(), rlen);
    endMessage();
    _socket->disconnectFromHost();
}

void Connection::updateWindow (int id, int layer, const QRect& bounds, int fill)
{
    startMessage(21);
    _stream << UPDATE_WINDOW_MSG;
    _stream << (qint32)id;
    _stream << (qint32)layer;
    write(bounds);
    _stream << (qint32)fill;
    endMessage();
}

void Connection::removeWindow (int id)
{
    startMessage(5);
    _stream << REMOVE_WINDOW_MSG;
    _stream << (qint32)id;
    endMessage();
}

void Connection::setContents (int id, const QRect& bounds, const QIntVector& contents)
{
    int size = contents.size();

    startMessage(13 + size*4);
    _stream << SET_CONTENTS_MSG;
    _stream << (qint32)id;
    write(bounds);
    for (int ii = 0; ii < size; ii++) {
        _stream << (qint32)contents.at(ii);
    }
    endMessage();
}

void Connection::moveContents (int id, const QRect& source, const QPoint& dest, int fill)
{
    startMessage(21);
    _stream << MOVE_CONTENTS_MSG;
    _stream << (qint32)id;
    write(source);
    _stream << (qint16)dest.x();
    _stream << (qint16)dest.y();
    _stream << (qint32)fill;
    endMessage();
}

void Connection::setCookie (const QString& name, const QString& value)
{
    // set in our local map
    _cookies.insert(name, value);

    // cookies require encryption
    bool ocrypto = _crypto;
    if (!ocrypto) {
        toggleCrypto();
    }

    QByteArray nbytes = name.toUtf8(), vbytes = value.toUtf8();
    int nlen = nbytes.length(), vlen = vbytes.length();
    startMessage(3 + nlen + vlen);
    _stream << SET_COOKIE_MSG;
    _stream << (quint16)nlen;
    _stream.writeRawData(nbytes.constData(), nlen);
    _stream.writeRawData(vbytes.constData(), vlen);
    endMessage();

    // restore the previous crypto setting
    if (!ocrypto) {
        toggleCrypto();
    }
}

void Connection::toggleCrypto ()
{
    startMessage(1);
    _stream << TOGGLE_CRYPTO_MSG;
    endMessage();

    // note for future messages
    _crypto = !_crypto;
}

void Connection::startCompound ()
{
    if (_compoundCount++ != 0) {
        return;
    }

    // replace the socket with a buffer that will write to a byte array
    QBuffer* buffer = new QBuffer();
    buffer->open(QIODevice::WriteOnly);
    _stream.setDevice(buffer);
}

void Connection::commitCompound ()
{
    if (--_compoundCount != 0) {
        return;
    }

    // restore the socket, write the data, delete the buffer
    QBuffer* buffer = static_cast<QBuffer*>(_stream.device());
    _stream.setDevice(_socket);

    const QByteArray& bytes = buffer->buffer();
    int blen = bytes.length();
    if (blen > 0) {
        startMessage(1 + blen);
        _stream << COMPOUND_MSG;
        _stream.writeRawData(bytes.constData(), blen);
        endMessage();
    }

    delete buffer;
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
        unget(_socket, length);
        unget(_socket, PROTOCOL_VERSION);
        unget(_socket, PROTOCOL_MAGIC);
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
    QByteArray output = readEncrypted(length - 4 - 128);
    QDataStream ostream(output);
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
    _socket->disconnect(this, SLOT(readHeader()));

    // log the establishment
    qDebug() << "Connection established." << _displaySize;

    // notify the connection manager, which will create a session for the connection
    _app->connectionManager()->connectionEstablished(this);
}

void Connection::readMessages ()
{
    // read as many messages as are available
    while (true) {
        if (_clientCrypto) {
            // all messages fit within a single block, so that's exactly the size we want
            int blockSize = EVP_CIPHER_CTX_block_size(&_dctx);
            if (_socket->bytesAvailable() < blockSize) {
                return;
            }
            // read the block in and decrypt it
            QByteArray decrypted = readEncrypted(blockSize);
            QDataStream stream(decrypted);
            maybeReadMessage(stream, blockSize);

        } else if (!maybeReadMessage(_stream, _socket->bytesAvailable())) {
            return; // wait for the rest of the message
        }
    }
}

void Connection::ping ()
{
    startMessage(9);
    _stream << PING_MSG;
    _stream << currentTimeMillis();
    endMessage();
}

void Connection::close ()
{
    // log the destruction
    QString error;
    QDebug base = qDebug() << "Connection closed." << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }

    // we need nothing else from the socket
    _socket->disconnect(this);

    // clear the shared pointer so that we can be deleted
    _pointer.clear();

    // notify listeners that we've been closed
    emit closed();
}

void Connection::startMessage (quint16 length)
{
    if (_crypto && _compoundCount == 0) {
        // replace the socket with a buffer that will write to a byte array
        QBuffer* buffer = new QBuffer();
        buffer->open(QIODevice::WriteOnly);
        _stream.setDevice(buffer);

    } else {
        _stream << length;
    }
}

void Connection::endMessage ()
{
    if (_crypto && _compoundCount == 0) {
        // restore the socket, write the encrypted data, delete the buffer
        QBuffer* buffer = static_cast<QBuffer*>(_stream.device());
        _stream.setDevice(_socket);
        writeEncrypted(buffer->buffer());
        delete buffer;
    }
}

void Connection::write (const QRect& rect)
{
    _stream << (qint16)rect.left();
    _stream << (qint16)rect.top();
    _stream << (qint16)rect.width();
    _stream << (qint16)rect.height();
}

QByteArray Connection::readEncrypted (int length)
{
    QByteArray in = _socket->read(length);
    QByteArray out(in.length(), 0);
    int outl;
    EVP_DecryptUpdate(&_dctx, (unsigned char*)out.data(), &outl,
        (unsigned char*)in.data(), in.length());
    EVP_DecryptFinal_ex(&_dctx, (unsigned char*)out.data() + outl, &outl);

    // we immediately reinitialize using the last encrypted block as an initialization vector
    EVP_DecryptInit_ex(&_dctx, 0, 0, 0,
        (unsigned char*)in.data() + length - EVP_CIPHER_CTX_block_size(&_dctx));
    return out;
}

void Connection::writeEncrypted (QByteArray& in)
{
    int blockSize = EVP_CIPHER_CTX_block_size(&_ectx);
    QByteArray out(in.length() + blockSize, 0);
    int outl;
    EVP_EncryptUpdate(&_ectx, (unsigned char*)out.data(), &outl,
        (unsigned char*)in.data(), in.length());
    int finall;
    EVP_EncryptFinal_ex(&_ectx, (unsigned char*)out.data() + outl, &finall);
    int length = outl + finall;
    _stream << (quint16)length;
    _socket->write(out.constData(), length);

    // immediately reinitialize
    EVP_EncryptInit_ex(&_ectx, 0, 0, 0, (unsigned char*)out.data() + length - blockSize);
}

bool Connection::maybeReadMessage (QDataStream& stream, qint64 available)
{
    if (available < 1) {
        return false; // wait until we have the type
    }
    quint8 type;
    stream >> type;
    if (type == WINDOW_CLOSED_MSG) {
        emit windowClosed();

    } else if (type == CRYPTO_TOGGLED_MSG) {
        _clientCrypto = !_clientCrypto;

    } else if (type == PONG_MSG) {
        if (available < 9) {
            _socket->ungetChar(type);
            return false; // wait until we have the data
        }
        quint64 then;
        stream >> then;
        qDebug() << "rtt" << (currentTimeMillis() - then);

    } else if (type == MOUSE_PRESSED_MSG || type == MOUSE_RELEASED_MSG) {
        if (available < 5) {
            _socket->ungetChar(type);
            return false; // wait until we have the data
        }
        quint16 x, y;
        stream >> x;
        stream >> y;
        emit (type == MOUSE_PRESSED_MSG) ? mousePressed(x, y) : mouseReleased(x, y);

    } else {
        // type == KEY_PRESSED_MSG || type == KEY_PRESSED_NUMPAD_MSG ||
        // type == KEY_RELEASED_MSG || type == KEY_RELEASED_NUMPAD_MSG
        if (available < 7) {
            _socket->ungetChar(type);
            return false; // wait until we have the data
        }
        quint32 key;
        quint16 ch;
        stream >> key;
        stream >> ch;
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
    // make sure they're not exceeding the message limit
    if (!_throttle.attemptOp()) {
        qDebug() << "Message limit exceeded." << _address;
        deactivate("Message limit exceeded.");
    }
    return true;
}
