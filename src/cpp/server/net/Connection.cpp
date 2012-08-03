//
// $Id$

#include <iostream>

#include <math.h>

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
#include "http/HttpConnection.h"
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

const QMetaMethod& Connection::reconnectMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("reconnect(QString,quint16)"));
    return method;
}

const QMetaMethod& Connection::evaluateMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("evaluate(QString)"));
    return method;
}

/**
 * Converts degrees to radians.
 */
static inline double toRadians (double value)
{
    return value * 3.14159265358979323846 / 180;
}

/**
 * Contains the name and location of a region.
 */
class Region {
public:

    /** The region's name. */
    QString name;

    /** The region's latitude in degrees north. */
    double latitude;

    /** The region's longitude in degrees east. */
    double longitude;

    /**
     * Returns the angular distance (assuming a unit sphere) to this region from the specified
     * location.
     */
    double distance (double olatitude, double olongitude) const
    {
        double tlat = toRadians(latitude);
        double tlon = toRadians(longitude);
        double olat = toRadians(olatitude);
        double olon = toRadians(olongitude);
        return acos(
            cos(tlat)*cos(tlon) * cos(olat)*cos(olon) + // x
            cos(tlat)*sin(tlon) * cos(olat)*sin(olon) + // y
            sin(tlat) * sin(olat)); // z
    }
};

/** The available regions and their locations. */
static const Region Regions[] = { {"us-east", 39.0437, -77.4875} };

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    _pointer(this, &QObject::deleteLater),
    _app(app),
    _socket(socket),
    _httpConnection(0),
    _address(socket->peerAddress()),
    _stream(socket),
    _crypto(false),
    _clientCrypto(false),
    _compoundCount(0),
    _throttle(50, 1000)
{
    init();

    // take over ownership of the socket and set its low delay option
    _socket->setParent(this);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(emitReadyRead()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(close()));
    connect(socket, SIGNAL(disconnected()), SLOT(close()));
    connect(this, SIGNAL(windowClosed()), SLOT(close()));

    // log the connection
    qDebug() << "Connection opened." << _address;
}

Connection::Connection (ServerApp* app, HttpConnection* httpConnection) :
    _pointer(this, &QObject::deleteLater),
    _app(app),
    _socket(httpConnection->socket()),
    _httpConnection(httpConnection),
    _address(_socket->peerAddress()),
    _stream(_socket),
    _crypto(false),
    _clientCrypto(false),
    _compoundCount(0),
    _throttle(50, 1000)
{
    init();

    // take over ownership of the HTTP connection
    _httpConnection->setParent(this);

    // connect initial slots
    connect(httpConnection, SIGNAL(webSocketMessageAvailable(QIODevice*,int,bool)),
        SLOT(readWebSocketMessage(QIODevice*,int,bool)));
    connect(httpConnection, SIGNAL(webSocketClosed(quint16,QByteArray)), SLOT(close()));

    // log the connection
    qDebug() << "WebSocket connection opened." << _address;
}

Connection::~Connection ()
{
    // cleanup crypto bits
    EVP_CIPHER_CTX_cleanup(&_ectx);
    EVP_CIPHER_CTX_cleanup(&_dctx);

    // and geoip bits
    if (_geoIpRecord != 0) {
        GeoIPRecord_delete(_geoIpRecord);
    }
}

void Connection::activate ()
{
    // connect the signal in order to read incoming messages
    connect(this, SIGNAL(readyRead(int)), SLOT(readMessages(int)));

    if (_httpConnection != 0) {
        // unpause WebSocket, processing buffered messages
        _httpConnection->setWebSocketPaused(false);

    } else {
        // read any messages already available
        readMessages(_socket->bytesAvailable());
    }
}

void Connection::deactivate (const QString& reason)
{
    QByteArray rbytes = reason.toUtf8();
    int rlen = rbytes.length();
    startMessage(1 + rlen);
    _stream << CLOSE_MSG;
    _stream.writeRawData(rbytes.constData(), rlen);
    endMessage();

    if (_httpConnection != 0) {
        _httpConnection->closeWebSocket();
    } else {
        _socket->disconnectFromHost();
    }
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

void Connection::reconnect (const QString& host, quint16 port)
{
    QByteArray hbytes = host.toUtf8();
    int hlen = hbytes.length();
    startMessage(3 + hlen);
    _stream << RECONNECT_MSG;
    _stream.writeRawData(hbytes.constData(), hlen);
    _stream << port;
    endMessage();

    if (_httpConnection != 0) {
        _httpConnection->closeWebSocket();
    } else {
        _socket->disconnectFromHost();
    }
}

void Connection::evaluate (const QString& expression)
{
    QByteArray ebytes = expression.toUtf8();
    int elen = ebytes.length();
    startMessage(1 + elen);
    _stream << EVALUATE_MSG;
    _stream.writeRawData(ebytes.constData(), elen);
    endMessage();
}

void Connection::emitReadyRead ()
{
    emit readyRead(_socket->bytesAvailable());
}

void Connection::readHeader (int bytesAvailable)
{
    // if we don't have the full header, wait until we do
    if (bytesAvailable < 12) {
        return;
    }
    // check the magic number and version
    quint32 magic, version, length;
    _stream >> magic;
    if (magic == 0x3C706F6C) { // "<pol"
        // surprise!  it's a flash policy file request; return a fixed policy
        qDebug() << "Got Flash policy request." << _socket->peerAddress();
        _socket->write(
            "<cross-domain-policy>\n"
            "  <allow-access-from domain=\"*\" to-ports=\"*\"/>\n"
            "</cross-domain-policy>\n");
        disconnect(this, SLOT(readHeader(int)));
        _socket->disconnectFromHost();
        return;
    }
    if (magic != PROTOCOL_MAGIC) {
        qWarning() << "Invalid protocol magic number:" << magic << _address;
        deactivate("Invalid magic number.");
        return;
    }
    _stream >> version;
    if (version != PROTOCOL_VERSION) {
        qWarning() << "Wrong protocol version:" << version << _address;
        deactivate("Wrong protocol version.");
        return;
    }
    _stream >> length;
    if (bytesAvailable - 12 < length) {
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

    QByteArray encryptedSecret = _stream.device()->read(128);
    QByteArray secret(32, 0);
    int len = RSA_private_decrypt(128, (const unsigned char*)encryptedSecret.constData(),
        (unsigned char*)secret.data(), _app->connectionManager()->rsa(), RSA_PKCS1_PADDING);
    if (len != 32) {
        qWarning() << "Invalid encryption key:" << _address;
        deactivate("Invalid encryption key.");
        return;
    }
    EVP_EncryptInit_ex(&_ectx, EVP_aes_128_cbc(), 0, (unsigned char*)secret.data(),
        (unsigned char*)secret.data() + 16);
    EVP_DecryptInit_ex(&_dctx, EVP_aes_128_cbc(), 0, (unsigned char*)secret.data(),
        (unsigned char*)secret.data() + 16);

    // the rest is encrypted with the secret key
    QByteArray output = readEncrypted(length - 4 - 128);
    QDataStream stream(output);

    quint16 qlen, clen;
    stream >> qlen;
    QByteArray query(qlen, 0);
    stream.readRawData(query.data(), qlen);
    stream >> clen;
    QByteArray cookie(clen, 0);
    stream.readRawData(cookie.data(), clen);

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
    disconnect(this, SLOT(readHeader(int)));
    if (_httpConnection != 0) {
        _httpConnection->setWebSocketPaused(true);
    }

    // log the establishment
    qDebug() << "Connection established." << _displaySize;

    // notify the connection manager, which will create a session for the connection
    _app->connectionManager()->connectionEstablished(this);
}

void Connection::readMessages (int bytesAvailable)
{
    // read as many messages as are available
    while (true) {
        if (_clientCrypto) {
            // all messages fit within a single block, so that's exactly the size we want
            int blockSize = EVP_CIPHER_CTX_block_size(&_dctx);
            if (bytesAvailable < blockSize) {
                return;
            }
            // read the block in and decrypt it
            QByteArray decrypted = readEncrypted(blockSize);
            bytesAvailable -= blockSize;
            QDataStream stream(decrypted);
            maybeReadMessage(stream, blockSize);

        } else if (!maybeReadMessage(_stream, bytesAvailable)) {
            return; // wait for the rest of the message
        }
    }
}

void Connection::readWebSocketMessage (QIODevice* device, int length, bool text)
{
    if (text) {
        qWarning() << "Got text message over WebSocket." << device->read(length);
    } else {
        _stream.setDevice(device);
        emit readyRead(length);
        _stream.setDevice(_socket);
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

void Connection::init ()
{
    // init crypto bits
    EVP_CIPHER_CTX_init(&_ectx);
    EVP_CIPHER_CTX_init(&_dctx);

    // and geoip/region bits
    _geoIpRecord = GeoIP_record_by_ipnum(_app->connectionManager()->geoIp(),
        _address.toIPv4Address());
    _region = Regions[0].name;
    if (_geoIpRecord != 0) {
        // find the closest region
        double leastDist = Regions[0].distance(_geoIpRecord->latitude, _geoIpRecord->longitude);
        for (int ii = 1, nn = sizeof(Regions) / sizeof(Region); ii < nn; ii++) {
            double dist = Regions[ii].distance(_geoIpRecord->latitude, _geoIpRecord->longitude);
            if (dist < leastDist) {
                _region = Regions[ii].name;
                leastDist = dist;
            }
        }
    }

    // set the socket's low delay option, in case it helps with latency
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // connect initial slot
    connect(this, SIGNAL(readyRead(int)), SLOT(readHeader(int)));
}

void Connection::startMessage (quint16 length)
{
    if (_compoundCount == 0) {
        if (_crypto) {
            // replace the socket with a buffer that will write to a byte array
            QBuffer* buffer = new QBuffer();
            buffer->open(QIODevice::WriteOnly);
            _stream.setDevice(buffer);
            return;
        }
        if (_httpConnection != 0) {
            _httpConnection->writeWebSocketHeader(length);
            return;
        }
    }
    _stream << length;
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
    QByteArray in = _stream.device()->read(length);
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
    if (_httpConnection != 0) {
        _httpConnection->writeWebSocketHeader(length);
    } else {
        _stream << (quint16)length;
    }
    _socket->write(out.constData(), length);

    // immediately reinitialize
    EVP_EncryptInit_ex(&_ectx, 0, 0, 0, (unsigned char*)out.data() + length - blockSize);
}

bool Connection::maybeReadMessage (QDataStream& stream, int& available)
{
    if (available < 1) {
        return false; // wait until we have the type
    }
    quint8 type;
    stream >> type;
    if (type == WINDOW_CLOSED_MSG) {
        available--;
        emit windowClosed();

    } else if (type == CRYPTO_TOGGLED_MSG) {
        available--;
        _clientCrypto = !_clientCrypto;

    } else if (type == PONG_MSG) {
        if (available < 9) {
            _socket->ungetChar(type);
            return false; // wait until we have the data
        }
        quint64 then;
        stream >> then;
        available -= 9;
        qDebug() << "rtt" << (currentTimeMillis() - then);

    } else if (type == MOUSE_PRESSED_MSG || type == MOUSE_RELEASED_MSG) {
        if (available < 5) {
            _socket->ungetChar(type);
            return false; // wait until we have the data
        }
        quint16 x, y;
        stream >> x;
        stream >> y;
        available -= 5;
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
        available -= 7;
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
