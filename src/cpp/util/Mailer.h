//
// $Id$

#ifndef MAILER
#define MAILER

#include <QRunnable>
#include <QString>

#include "util/Callback.h"

class QTcpSocket;

/**
 * Used to send an email.
 */
class Mailer : public QRunnable
{
public:

    /**
     * Creates a new mailer.
     */
    Mailer (const QString& hostname, quint16 port, const QString& from, const QString& to,
        const QString& subject, const QString& body, const Callback& callback);

    /**
     * Sends the email.
     */
    virtual void run ();

protected:

    /**
     * Sends a single command and waits for the result, sending it to the callback on error.
     *
     * @param result the expected result code.
     * @return whether or not the command succeeded.
     */
    bool send (QTcpSocket& socket, const QByteArray& command, const QByteArray& result);

    /**
     * Waits for a line from the server, sending it to the callback on error.
     *
     * @param result the expected result code.
     * @return whether or not the receipt succeeded.
     */
    bool receive (QTcpSocket& socket, const QByteArray& result);

    /** The hostname to which we will connect. */
    QString _hostname;

    /** The port on which we will connect. */
    quint16 _port;

    /** The from address. */
    QString _from;

    /** The to address. */
    QString _to;

    /** The email subject. */
    QString _subject;

    /** The email body. */
    QString _body;

    /** The callback to notify on completion. */
    Callback _callback;
};

#endif // MAILER
