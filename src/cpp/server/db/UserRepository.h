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
     * Returns a reference to the blocked name expression.
     */
    const QRegExp& blockedNameExp () const { return _blockedNameExp; }

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Attempts to insert a new user record.
     *
     * @param callback the callback that will be invoked with (quint32) id of the newly inserted
     * user, or 0 if the name was already taken.
     */
    Q_INVOKABLE void insertUser (
        quint64 sessionId, const QString& name, const QString& password, const QDate& dob,
        const QString& email, QChar avatar, const Callback& callback);

    /**
     * Attempts to validate a user logon.
     *
     * @param callback the callback that will be invoked with a QVariant containing either the
     * {@link #LogonError} indicating why the logon was denied, or a {@link UserRecord} containing
     * the user's information if successful.
     */
    Q_INVOKABLE void validateLogon (
        const QString& name, const QString& password, const Callback& callback);

    /**
     * Loads the record for the named user and provides it to the specified callback.
     */
    Q_INVOKABLE void loadUser (const QString& name, const Callback& callback);

    /**
     * Attempts to find the username corresponding to the specified email.  Returns an empty string
     * if not found.
     */
    Q_INVOKABLE void usernameForEmail (const QString& email, const Callback& callback);

    /**
     * Loads the record for the identified user.
     */
    UserRecord loadUser (quint32 id);

    /**
     * Updates a user record.  The callback will receive a bool indicating whether the update was
     * successful (failure may occur due to a username collision).
     */
    Q_INVOKABLE void updateUser (const UserRecord& urec, const Callback& callback);

    /**
     * Deletes the identified user.
     */
    Q_INVOKABLE void deleteUser (quint32 id);

    /**
     * Attempts to insert a password reset record for the identified user.  The callback will
     * receive the reset id and token.
     */
    Q_INVOKABLE void insertPasswordReset (quint32 userId, const Callback& callback);

    /**
     * Attempts to validate a password reset.  The callback will receive a QVariant containing
     * either the UserRecord or the error code (as in {@link #validateLogon}).
     */
    Q_INVOKABLE void validatePasswordReset (
        quint32 id, const QByteArray& token, const Callback& callback);

    /**
     * Helper function for logon methods; validates the logon for the specified user and returns
     * the result to provide to the callbac.
     */
    QVariant validateLogon (const UserRecord& urec);

protected:

    /** The server application. */
    ServerApp* _app;

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
    STREAM quint32 id;

    /** The cased username. */
    STREAM QString name;

    /** The password hash. */
    STREAM QByteArray passwordHash;

    /** The password salt. */
    STREAM QByteArray passwordSalt;

    /** The user's date of birth. */
    STREAM QDate dateOfBirth;

    /** The user's email. */
    STREAM QString email;

    /** The user's flags. */
    STREAM Flags flags;

    /** The user's avatar. */
    STREAM QChar avatar;

    /** The time at which the user was created. */
    STREAM QDateTime created;

    /** The time at which the user was last online. */
    STREAM QDateTime lastOnline;

    /** The zone occupied by the user. */
    STREAM quint32 lastZoneId;

    /** The scene last occupied by the user. */
    STREAM quint32 lastSceneId;

    /**
     * Sets the password hash.
     */
    void setPassword (const QString& password);

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
