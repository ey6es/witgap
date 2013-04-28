//
// $Id$

#ifndef USER_REPOSITORY
#define USER_REPOSITORY

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QMetaType>
#include <QObject>
#include <QRegExp>
#include <QString>

#include "util/Streaming.h"

class Callback;
class ServerApp;
class UserRecord;

/**
 * Handles database queries associated with users.
 */
class UserRepository : public QObject
{
    Q_OBJECT

public:

    /** The possible errors in logon validation. */
    enum LogonError { NoError, NoSuchUser, WrongPassword, Banned, ServerClosed };

    /**
     * Creates the user repository.
     */
    UserRepository (ServerApp* app);

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Validates the specified session token.
     *
     * @param callback the callback that will be invoked with the {@link UserRecord} of the user
     * associated with the session.
     */
    Q_INVOKABLE void validateSessionToken (
        quint64 userId, const QByteArray& token, const Callback& callback);

    /**
     * Creates a new, passwordless user.
     *
     * @param callback the callback that will be invoked with the new {@link UserRecord} of the
     * user associated with the session.
     */
    Q_INVOKABLE void createUser (const Callback& callback);

    /**
     * Attempts to validate a user logon.
     *
     * @param orec the current, passwordless user record.
     * @param callback the callback that will be invoked with a QVariant containing either the
     * {@link #LogonError} indicating why the logon was denied, or a {@link UserRecord} containing
     * the user's information if successful.
     */
    Q_INVOKABLE void validateLogon (
        const UserRecord& orec, const QString& name,
        const QString& password, const Callback& callback);

    /**
     * Loads the record for the named user and provides it to the specified callback.
     */
    Q_INVOKABLE void loadUser (const QString& name, const Callback& callback);

    /**
     * Loads the record for the user with the supplied address and provides it to the specified
     * callback.
     */
    Q_INVOKABLE void loadUserByEmail (const QString& email, const Callback& callback);

    /**
     * Updates a user record.  The callback will receive a bool indicating whether the update was
     * successful (failure may occur due to a username collision).
     */
    Q_INVOKABLE void updateUser (const UserRecord& urec, const Callback& callback);

    /**
     * Deletes the identified user.
     */
    Q_INVOKABLE void deleteUser (quint64 id);

    /**
     * Attempts to insert a password reset record for the identified user.  The callback will
     * receive the reset URL as a string.
     */
    Q_INVOKABLE void insertPasswordReset (quint64 userId, const Callback& callback);

    /**
     * Attempts to validate a password reset.  The callback will receive a QVariant containing
     * either the UserRecord or the error code (as in {@link #validateLogon}).
     *
     * @param orec the current, most likely passwordless, user record.
     */
    Q_INVOKABLE void validatePasswordReset (
        const UserRecord& orec, quint32 id, const QByteArray& token, const Callback& callback);

    /**
     * Inserts an invite.  The callback will receive the invite URL as a string.
     */
    Q_INVOKABLE void insertInvite (const QString& description, int flags,
        int count, const Callback& callback);

    /**
     * Attempts to validate an invite.  The callback will receive a boolean indicating validity
     * (or lack thereof).
     */
    Q_INVOKABLE void validateInvite (
        quint32 id, const QByteArray& token, const Callback& callback);

protected:

    /**
     * Logs a message with the initial admin bootstrap invite URL.
     */
    Q_INVOKABLE void reportBootstrapInvite (const QString& url);

    /**
     * Generates a random name that isn't used by any session/user.
     */
    QString uniqueRandomName () const;

    /**
     * Generates a random name using the loaded Markov chain probabilities.
     */
    QString randomName () const;

    /**
     * Helper function for logon methods; determines whether the described user can log on.
     */
    LogonError validateLogon (const UserRecord& urec);

    /**
     * Helper function for logon functions; deletes the old user if appropriate, updates the
     * timestamp, invokes the callback.
     */
    void logon (const UserRecord& orec, UserRecord& nrec, const Callback& callback);

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

    /** The expression that matches blocked names. */
    QRegExp _blockedNameExp;
};

/**
 * Contains user information loaded from the database.
 */
class UserRecord
{
    STREAMABLE

public:

    /** User flags.  Do not change. */
    enum Flag { NoFlag = 0x0, Banned = 0x1, Admin = 0x2, Insider = 0x4 };

    Q_DECLARE_FLAGS(Flags, Flag)

    /** The user id. */
    STREAM quint64 id;

    /** The session token. */
    STREAM QByteArray sessionToken;

    /** The cased username. */
    STREAM QString name;

    /** The user's avatar. */
    STREAM QChar avatar;

    /** The time at which the user was created. */
    STREAM QDateTime created;

    /** The time at which the user was last online. */
    STREAM QDateTime lastOnline;

    /** The password salt. */
    STREAM QByteArray passwordSalt;

    /** The password hash. */
    STREAM QByteArray passwordHash;

    /** The user's date of birth. */
    STREAM QDate dateOfBirth;

    /** The user's email. */
    STREAM QString email;

    /** The user's flags. */
    STREAM Flags flags;

    /** The zone occupied by the user. */
    STREAM quint32 lastZoneId;

    /** The scene last occupied by the user. */
    STREAM quint32 lastSceneId;

    /**
     * Sets the password hash.
     */
    void setPassword (const QString& password);

    /**
     * Checks whether the user is logged on (as opposed to an anonymous account).
     */
    bool loggedOn () const { return !passwordHash.isEmpty(); }

    /**
     * Checks whether the user is at least an insider.
     */
    bool insiderPlus () const { return flags.testFlag(Insider) || flags.testFlag(Admin); }
};

DECLARE_STREAMABLE_METATYPE(UserRecord)
DECLARE_STREAMING_OPERATORS_FOR_FLAGS(UserRecord::Flags)

/** A record for the lack of a user. */
const UserRecord NoUser = { 0 };

#endif // USER_REPOSITORY
