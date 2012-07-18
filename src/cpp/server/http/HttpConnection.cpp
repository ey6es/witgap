//
// $Id$

#include <QCryptographicHash>
#include <QTcpSocket>

#include "ServerApp.h"
#include "http/HttpConnection.h"
#include "http/HttpManager.h"

HttpConnection::HttpConnection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->httpManager()),
    _app(app),
    _socket(socket),
    _masker(new MaskFilter(socket, this)),
    _stream(socket),
    _address(socket->peerAddress())
{
    // take over ownership of the socket
    _socket->setParent(this);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readRequest()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));

    // log the connection
    qDebug() << "HTTP connection opened." << _address;
}

HttpConnection::~HttpConnection ()
{
    // log the destruction
    QString error;
    QDebug base = qDebug() << "HTTP connection closed." << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
}

void HttpConnection::respond (
    const char* code, const QByteArray& content, const char* contentType, const Headers& headers)
{
    _socket->write("HTTP/1.1 ");
    _socket->write(code);
    _socket->write("\r\n");

    int csize = content.size();

    for (Headers::const_iterator it = headers.constBegin(), end = headers.constEnd();
            it != end; it++) {
        _socket->write(it.key());
        _socket->write(": ");
        _socket->write(it.value());
        _socket->write("\r\n");
    }
    if (csize > 0) {
        _socket->write("Content-Length: ");
        _socket->write(QByteArray::number(csize));
        _socket->write("\r\n");

        _socket->write("Content-Type: ");
        _socket->write(contentType);
        _socket->write("\r\n");
    }
    _socket->write("Connection: close\r\n\r\n");

    if (csize > 0) {
        _socket->write(content);
    }

    // make sure we receive no further read notifications
    _socket->disconnect(SIGNAL(readyRead()), this);

    _socket->disconnectFromHost();
}

void HttpConnection::switchToWebSocket (const char* protocol)
{
    _socket->write("HTTP/1.1 101 Switching Protocols\r\n");
    _socket->write("Upgrade: websocket\r\n");
    _socket->write("Connection: Upgrade\r\n");
    _socket->write("Sec-WebSocket-Accept: ");

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(_requestHeaders.value("Sec-WebSocket-Key"));
    hash.addData("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); // from WebSocket draft RFC
    _socket->write(hash.result().toBase64());

    _socket->write("\r\nSec-WebSocket-Protocol: ");
    _socket->write(protocol);
    _socket->write("\r\n\r\n");

    connect(_socket, SIGNAL(readyRead()), SLOT(readFrame()));
}

void HttpConnection::readRequest ()
{
    if (!_socket->canReadLine()) {
        return;
    }
    // parse out the method and resource
    QByteArray line = _socket->readLine().trimmed();
    if (line.startsWith("HEAD")) {
        _requestOperation = QNetworkAccessManager::HeadOperation;

    } else if (line.startsWith("GET")) {
        _requestOperation = QNetworkAccessManager::GetOperation;

    } else if (line.startsWith("PUT")) {
        _requestOperation = QNetworkAccessManager::PutOperation;

    } else if (line.startsWith("POST")) {
        _requestOperation = QNetworkAccessManager::PostOperation;

    } else if (line.startsWith("DELETE")) {
        _requestOperation = QNetworkAccessManager::DeleteOperation;

    } else {
        qWarning() << "Unrecognized HTTP operation." << _address << line;
        respond("400 Bad Request", "Unrecognized operation.");
        return;
    }
    int idx = line.indexOf(' ') + 1;
    _requestUrl = QUrl(line.mid(idx, line.lastIndexOf(' ') - idx));

    // switch to reading the header
    _socket->disconnect(this, SLOT(readRequest()));
    connect(_socket, SIGNAL(readyRead()), SLOT(readHeaders()));

    // read any headers immediately available
    readHeaders();
}

void HttpConnection::readHeaders ()
{
    while (_socket->canReadLine()) {
        QByteArray line = _socket->readLine();
        QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            _socket->disconnect(this, SLOT(readHeaders()));

            QByteArray clength = _requestHeaders.value("Content-Length");
            if (clength.isEmpty()) {
                _app->httpManager()->handleRequest(this);

            } else {
                _requestContent.resize(clength.toInt());
                connect(_socket, SIGNAL(readyRead()), SLOT(readContent()));

                // read any content immediately available
                readContent();
            }
            return;
        }
        char first = line.at(0);
        if (first == ' ' || first == '\t') { // continuation
            _requestHeaders[_lastRequestHeader].append(trimmed);
            continue;
        }
        int idx = trimmed.indexOf(':');
        if (idx == -1) {
            qWarning() << "Invalid header." << _address << trimmed;
            respond("400 Bad Request", "The header was malformed.");
            return;
        }
        _lastRequestHeader = trimmed.left(idx);
        QByteArray& value = _requestHeaders[_lastRequestHeader];
        if (!value.isEmpty()) {
            value.append(", ");
        }
        value.append(trimmed.mid(idx + 1).trimmed());
    }
}

void HttpConnection::readContent ()
{
    int size = _requestContent.size();
    if (_socket->bytesAvailable() < size) {
        return;
    }
    _socket->read(_requestContent.data(), size);
    _socket->disconnect(this, SLOT(readContent()));

    _app->httpManager()->handleRequest(this);
}

void HttpConnection::readFrame ()
{
    // make sure we have at least the first two bytes
    qint64 available = _socket->bytesAvailable();
    if (available < 2) {
        return;
    }
    // read the first two, which tell us whether we need more for the length
    quint8 finalOpcode, maskLength;
    _stream >> finalOpcode;
    _stream >> maskLength;
    available -= 2;

    int byteLength = maskLength & 0x7F;
    bool masked = (maskLength & 0x80) != 0;
    int baseLength = (masked ? 4 : 0);
    int length = -1;
    if (byteLength == 127) {
        if (available >= 8) {
            quint64 longLength;
            _stream >> longLength;
            if (available >= baseLength + 8 + longLength) {
                length = longLength;
            } else {
                unget(_socket, longLength & 0xFFFFFFFF);
                unget(_socket, longLength >> 32);
            }
        }
    } else if (byteLength == 126) {
        if (available >= 2) {
            quint16 shortLength;
            _stream >> shortLength;
            if (available >= baseLength + 2 + shortLength) {
                length = shortLength;
            } else {
                _socket->ungetChar(shortLength & 0xFF);
                _socket->ungetChar(shortLength >> 8);
            }
        }
    } else if (available >= baseLength + byteLength) {
        length = byteLength;
    }
    if (length == -1) {
        _socket->ungetChar(maskLength);
        _socket->ungetChar(finalOpcode);
        return;
    }

    // read the mask and set it in the filter
    quint32 mask = 0;
    if (masked) {
        _stream >> mask;
    }
    _masker->setMask(mask);


}

void HttpConnection::writeFrameHeader (FrameOpcode opcode, int size, bool final)
{
    _socket->putChar((final ? 0x80 : 0x0) | opcode);
    if (size < 126) {
        _socket->putChar(size);

    } else if (size < 65536) {
        _socket->putChar(126);
        _stream << (quint16)size;

    } else {
        _socket->putChar(127);
        _stream << (quint64)size;
    }
}

MaskFilter::MaskFilter (QIODevice* device, QObject* parent) :
    QIODevice(parent),
    _device(device)
{
    open(ReadOnly);
}

void MaskFilter::setMask (quint32 mask)
{
    _mask[0] = (mask >> 24);
    _mask[1] = (mask >> 16) & 0xFF;
    _mask[2] = (mask >> 8) & 0xFF;
    _mask[3] = mask & 0xFF;
    _position = 0;
    reset();
}

qint64 MaskFilter::bytesAvailable () const
{
    return _device->bytesAvailable() + QIODevice::bytesAvailable();
}

qint64 MaskFilter::readData (char* data, qint64 maxSize)
{
    qint64 bytes = _device->read(data, maxSize);
    for (char* end = data + bytes; data < end; data++) {
        *data ^= _mask[_position];
        _position = (_position + 1) % 4;
    }
    return bytes;
}

qint64 MaskFilter::writeData (const char* data, qint64 maxSize)
{
    return _device->write(data, maxSize);
}
