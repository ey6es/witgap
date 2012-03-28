//
// $Id$

#ifndef SESSION_REPOSITORY
#define SESSION_REPOSITORY

#include <QDateTime>
#include <QMetaType>
#include <QObject>

class Callback;
class ServerApp;
class SessionRecord;

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
     * @param callback the callback that will be invoked with the {@link SessionRecord} of the
     * session and the {@link UserRecord} of the user associated with the session.
     */
    Q_INVOKABLE void validateToken (quint64 id, const QByteArray& token, const Callback& callback);

    /**
     * Updates a session record.
     */
    Q_INVOKABLE void updateSession (const SessionRecord& session);

protected:

    /** The server application. */
    ServerApp* _app;
};

/**
 * Contains session information loaded from the database.
 */
class SessionRecord
{
public:

    /** The session id. */
    quint64 id;

    /** The session token. */
    QByteArray token;

    /** The session avatar. */
    QChar avatar;

    /** The id of the associated user, or zero for none. */
    quint32 userId;

    /** The last online time. */
    QDateTime lastOnline;
};

Q_DECLARE_METATYPE(SessionRecord)

#endif // SESSION_REPOSITORY
