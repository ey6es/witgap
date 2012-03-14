//
// $Id$

#ifndef USER_REPOSITORY
#define USER_REPOSITORY

#include <QObject>
#include <QMetaType>
#include <QString>

class QDate;

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
        const QString& email, const Callback& callback);

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
     * Loads the record for the identified user.
     */
    UserRecord loadUser (quint32 id);
};

/**
 * Contains user information loaded from the database.
 */
class UserRecord
{
public:

    /** User flags.  Do not change. */
    enum Flag { Banned = 0x1, Admin = 0x2 };

    Q_DECLARE_FLAGS(Flags, Flag)

    /** The user id. */
    quint32 id;

    /** The cased username. */
    QString name;

    /** The user's flags. */
    Flags flags;
};

Q_DECLARE_METATYPE(UserRecord)
Q_DECLARE_OPERATORS_FOR_FLAGS(UserRecord::Flags)

/** A record for the lack of a user. */
const UserRecord NoUser = { 0 };

#endif // USER_REPOSITORY
