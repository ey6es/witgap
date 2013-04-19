//
// $Id$

#ifndef SESSION_REPOSITORY
#define SESSION_REPOSITORY

#include <QDateTime>
#include <QMetaType>
#include <QObject>

#include "util/Streaming.h"

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

    /**
     * Logs a session off.  The callback will receive the random name generated for the logged-off
     * session.
     */
    Q_INVOKABLE void logoffSession (quint64 id, const Callback& callback);

protected:

    /**
     * Generates a random name that isn't used by any session/user.
     */
    QString uniqueRandomName () const;

    /**
     * Generates a random name using the loaded Markov chain probabilities.
     */
    QString randomName () const;

    /** The server application. */
    ServerApp* _app;

    /** The minimum and maximum random name lengths. */
    static const int MinNameLength = 4, MaxNameLength = 12;

    /** Probabilities for each name length. */
    double _nameLengths[MaxNameLength - MinNameLength + 1];

    /** The number of states in the name chain (letters plus start/end). */
    static const int NameChainStates = 26 + 1;

    /** The Markov chain for random name letters. */
    double _nameChain[NameChainStates][NameChainStates];
};

/**
 * Contains session information loaded from the database.
 */
class SessionRecord
{
    STREAMABLE

public:

    /** The session id. */
    STREAM quint64 id;

    /** The session token. */
    STREAM QByteArray token;

    /** The session name. */
    STREAM QString name;

    /** The session avatar. */
    STREAM QChar avatar;

    /** The id of the associated user, or zero for none. */
    STREAM quint32 userId;

    /** The last online time. */
    STREAM QDateTime lastOnline;
    
    /** Whether or not to stay logged in after closing. */
    STREAM bool stayLoggedIn;
};

DECLARE_STREAMABLE_METATYPE(SessionRecord)

#endif // SESSION_REPOSITORY
