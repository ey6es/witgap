//
// $Id$

#ifndef USER_REPOSITORY
#define USER_REPOSITORY

#include <QDate>
#include <QObject>
#include <QString>

#include "util/Callback.h"

/**
 * Handles database queries associated with users.
 */
class UserRepository : public QObject
{
    Q_OBJECT

public:

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
     * @param callback the callback that will be invoked with...
     */
    Q_INVOKABLE void validateLogon (
        const QString& name, const QString& password, const Callback& callback);
};

#endif // USER_REPOSITORY
