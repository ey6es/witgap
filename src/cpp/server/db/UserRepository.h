//
// $Id$

#ifndef USER_REPOSITORY
#define USER_REPOSITORY

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QMetaType>
#include <QObject>
#include <QString>

class Callback;
class UserRecord;

/**
 * Handles database queries associated with users.
 */
class UserRepository : public QObject
{
    Q_OBJECT

public:

    /** The possible errors in logon validation. */
    enum LogonError { NoError, NoSuchUser, WrongPassword, Banned };

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
        const QString& name, const QString& password, const QDate& dob,
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
};

/**
 * Contains user information loaded from the database.
 */
class UserRecord
{
public:

    /** User flags.  Do not change. */
    enum Flag { NoFlag = 0x0, Banned = 0x1, Admin = 0x2 };

    Q_DECLARE_FLAGS(Flags, Flag)

    /** The user id. */
    quint32 id;

    /** The cased username. */
    QString name;

    /** The password hash. */
    QByteArray passwordHash;

    /** The password salt. */
    QByteArray passwordSalt;

    /** The user's date of birth. */
    QDate dateOfBirth;

    /** The user's email. */
    QString email;

    /** The user's flags. */
    Flags flags;

    /** The user's avatar. */
    QChar avatar;

    /** The time at which the user was created. */
    QDateTime created;

    /** The time at which the user was last online. */
    QDateTime lastOnline;

    /**
     * Sets the password hash.
     */
    void setPassword (const QString& password);
};

Q_DECLARE_METATYPE(UserRecord)
Q_DECLARE_OPERATORS_FOR_FLAGS(UserRecord::Flags)

/** A record for the lack of a user. */
const UserRecord NoUser = { 0 };

#endif // USER_REPOSITORY
