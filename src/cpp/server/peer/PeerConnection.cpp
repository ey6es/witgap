//
// $Id$

#include <QSslSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"

PeerConnection::PeerConnection (ServerApp* app, QSslSocket* socket) :
    QObject(app->peerManager()),
    _app(app),
    _socket(socket)
{
    // take over ownership of the socket
    _socket->setParent(this);

    // start encryption
//    _socket->startServerEncryption();

    // connect initial slots
    // connect(socket, SIGNAL(readyRead()), SLOT(readHeader()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));

    // log the connection
    qDebug() << "Peer connection opened." << (_address = _socket->peerAddress());
}

PeerConnection::~PeerConnection ()
{
    // log the destruction
    QString error;
    QDebug base = qDebug() << "Peer connection closed." << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
}
