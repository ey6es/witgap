//
// $Id$

#ifndef SESSION_REPOSITORY
#define SESSION_REPOSITORY

#include <QObject>

class Callback;
class ServerApp;

/**
 * Handles database queries associated with sessions.
 */
class SessionRepository : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates the session repository.
     */
    SessionRepository (ServerApp* app);

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Validates the specified session token.
     *
     * @param callback the callback that will be invoked with a valid id and token (either the ones
     * passed in, or a newly generated pair) and the {@link UserRecord} of the user associated with
     * the session.
     */
    Q_INVOKABLE void validateToken (quint64 id, const QByteArray& token, const Callback& callback);

    /**
     * Sets the user id for a session.
     *
     * @param userId the new user id, or zero for none.
     */
    Q_INVOKABLE void setUserId (quint64 id, quint32 userId);

protected:

    /** The server application. */
    ServerApp* _app;
};

#endif // SESSION_REPOSITORY
