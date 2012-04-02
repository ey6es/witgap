//
// $Id$

#include <QTcpSocket>

#include "util/Mailer.h"

Mailer::Mailer (const QString& hostname, quint16 port, const QString& from, const QString& to,
        const QString& subject, const QString& body, const Callback& callback) :
    _hostname(hostname),
    _port(port),
    _from(from),
    _to(to),
    _subject(subject),
    _body(body),
    _callback(callback)
{
}

void Mailer::run ()
{
    QTcpSocket socket;
    socket.connectToHost(_hostname, _port);
    if (!socket.waitForConnected()) {
        _callback.invoke(Q_ARG(const QString&, socket.errorString()));
        return;
    }
    QByteArray toAscii = _to.toAscii();
    if (!(receive(socket, "220") &&
            send(socket, "HELO localhost", "250") &&
            send(socket, "MAIL FROM:<" + _from.toAscii() + ">", "250") &&
            send(socket, "RCPT TO:<" + toAscii + ">", "250") &&
            send(socket, "DATA", "354") &&
            send(socket, "To: " + toAscii + "\r\nSubject: " + _subject.toAscii() + "\r\n\r\n" +
                _body.toAscii() + "\r\n.", "250") &&
            send(socket, "QUIT", "221"))) {
        return;
    }
    _callback.invoke(Q_ARG(const QString&, ""));
}

bool Mailer::send (QTcpSocket& socket, const QByteArray& command, const QByteArray& result)
{
    socket.write(command + "\r\n");
    return receive(socket, result);
}

bool Mailer::receive (QTcpSocket& socket, const QByteArray& result)
{
    while (!socket.canReadLine()) {
        if (!socket.waitForReadyRead()) {
            _callback.invoke(Q_ARG(const QString&, socket.errorString()));
            return false;
        }
    }
    QByteArray line = socket.readLine();
    if (!line.startsWith(result)) {
        _callback.invoke(Q_ARG(const QString&, line));
        return false;
    }
    return true;
}
