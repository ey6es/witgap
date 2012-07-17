//
// $Id$

#include <QTcpSocket>

#include "ServerApp.h"
#include "http/HttpConnection.h"
#include "http/HttpManager.h"

HttpConnection::HttpConnection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->httpManager()),
    _app(app),
    _socket(socket),
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
    _socket->write("Connection: close\r\n");
    _socket->write("\r\n");

    if (csize > 0) {
        _socket->write(content);
    }

    // make sure we receive no further read notifications
    _socket->disconnect(SIGNAL(readyRead()), this);

    _socket->disconnectFromHost();
}

void HttpConnection::readRequest ()
{
    if (!_socket->canReadLine()) {
        return;
    }
    // parse out the method and resource
    QByteArray line = _socket->readLine().trimmed();
    if (line.startsWith("HEAD")) {
        _operation = QNetworkAccessManager::HeadOperation;

    } else if (line.startsWith("GET")) {
        _operation = QNetworkAccessManager::GetOperation;

    } else if (line.startsWith("PUT")) {
        _operation = QNetworkAccessManager::PutOperation;

    } else if (line.startsWith("POST")) {
        _operation = QNetworkAccessManager::PostOperation;

    } else if (line.startsWith("DELETE")) {
        _operation = QNetworkAccessManager::DeleteOperation;

    } else {
        qWarning() << "Unrecognized HTTP operation." << _address << line;
        respond("400 Bad Request", "Unrecognized operation.");
        return;
    }
    int idx = line.indexOf(' ') + 1;
    _resource = line.mid(idx, line.lastIndexOf(' ') - idx);

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
                handleRequest();

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

    handleRequest();
}

void HttpConnection::handleRequest ()
{
    respond("404 Not Found", "Resource not found.");
}
