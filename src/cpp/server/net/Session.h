//
// $Id$

#ifndef SESSION
#define SESSION

#include <QObject>
#include <QSize>

class Connection;
class ServerApp;

/**
 * Handles a single user session.
 */
class Session : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the session.
     */
    Session (ServerApp* app, Connection* connection, quint64 id, const QByteArray& token);

    /**
     * Returns the session id.
     */
    quint64 id () const { return _id; }

    /**
     * Returns the session token.
     */
    const QByteArray& token () const { return _token; }

    /**
     * Returns a pointer to the connection, or zero if unconnected.
     */
    Connection* connection () const { return _connection; }

    /**
     * Replaces the session connection.
     */
    void setConnection (Connection* connection);

    /**
     * Increments the window id counter and returns its value.
     */
    int nextWindowId () { return ++_lastWindowId; }

    /**
     * Returns the size of the user's display.
     */
    const QSize& displaySize () const { return _displaySize; }

    /**
     * Shows a simple info dialog with the supplied message.
     *
     * @param title the title to use for the dialog, or empty string for none.
     * @param dismiss the text to use for the dismiss button, or empty string for default.
     */
    void showInfoDialog (
        const QString& message, const QString& title = "", const QString& dismiss = "");

protected slots:

    /**
     * Clears the connection pointer (because it has been destroyed).
     */
    void clearConnection ();

    /**
     * Dispatches a mouse pressed event.
     */
    void dispatchMousePressed (int x, int y);

    /**
     * Dispatches a mouse released event.
     */
    void dispatchMouseReleased (int x, int y);

    /**
     * Dispatches a key pressed event.
     */
    void dispatchKeyPressed (int key);

    /**
     * Dispatches a key released event.
     */
    void dispatchKeyReleased (int key);

protected:

    /** The server application. */
    ServerApp* _app;

    /** The session connection. */
    Connection* _connection;

    /** The session id. */
    quint64 _id;

    /** The session token. */
    QByteArray _token;

    /** The last window id assigned. */
    int _lastWindowId;

    /** The size of the user's display. */
    QSize _displaySize;

    /** The current set of key modifiers. */
    Qt::KeyboardModifiers _modifiers;
};

#endif // SESSION
