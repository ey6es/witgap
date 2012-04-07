//
// $Id$

#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

#include "db/UserRepository.h"
#include "util/Callback.h"
#include "util/General.h"

// register our types with the metatype system
int userRecordType = qRegisterMetaType<UserRecord>();

void UserRepository::init ()
{
    // create the tables if they don't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("USERS")) {
        qDebug() << "Creating USERS table.";
        query.exec(
            "create table USERS ("
                "ID int unsigned not null auto_increment primary key,"
                "NAME varchar(16) not null,"
                "NAME_LOWER varchar(16) not null unique,"
                "PASSWORD_HASH binary(16) not null,"
                "PASSWORD_SALT binary(8) not null,"
                "DATE_OF_BIRTH date not null,"
                "EMAIL varchar(255) not null,"
                "FLAGS int unsigned not null,"
                "AVATAR smallint unsigned not null,"
                "CREATED datetime not null,"
                "LAST_ONLINE datetime not null,"
                "index (EMAIL))");
    }

    if (!database.tables().contains("PASSWORD_RESETS")) {
        qDebug() << "Creating PASSWORD_RESETS table.";
        query.exec(
            "create table PASSWORD_RESETS ("
                "ID int unsigned not null auto_increment primary key,"
                "TOKEN binary(16) not null,"
                "USER_ID int unsigned not null,"
                "CREATED datetime not null,"
                "index (CREATED),"
                "index (USER_ID))");
    }
}

/**
 * Helper function that returns the hash of the plaintext password and the supplied salt.
 */
QByteArray hashPassword (const QString& password, const QByteArray& salt)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(password.toUtf8());
    hash.addData(salt);
    return hash.result();
}

void UserRepository::insertUser (
    const QString& name, const QString& password, const QDate& dob,
    const QString& email, QChar avatar, const Callback& callback)
{
    QSqlQuery query;
    QDateTime now = QDateTime::currentDateTime();

    // generate a random salt
    QByteArray salt(8, 0);
    for (int ii = 0; ii < 8; ii++) {
        salt[ii] = qrand() % 256;
    }

    // use it to hash the password
    QByteArray passwordHash = hashPassword(password, salt);

    query.prepare(
        "insert into USERS (NAME, NAME_LOWER, PASSWORD_HASH, PASSWORD_SALT, DATE_OF_BIRTH, "
        "EMAIL, FLAGS, AVATAR, CREATED, LAST_ONLINE) values (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(name.toLower());
    query.addBindValue(passwordHash);
    query.addBindValue(salt);
    query.addBindValue(dob);
    query.addBindValue(email.toLower());
    query.addBindValue(0);
    query.addBindValue(avatar.unicode());
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        callback.invoke(Q_ARG(const UserRecord&, NoUser));
        return;
    }
    UserRecord urec = {
        query.lastInsertId().toUInt(), name, passwordHash, salt, dob, email, 0, avatar, now, now };
    callback.invoke(Q_ARG(const UserRecord&, urec));
}

/**
 * Helper function for user load methods.
 */
static UserRecord loadUserRecord (const QString& field, const QVariant& value)
{
    // look up the user id, password hash and salt
    QSqlQuery query;
    query.prepare(
        "select ID, NAME, PASSWORD_HASH, PASSWORD_SALT, DATE_OF_BIRTH, EMAIL, FLAGS, AVATAR, "
        "CREATED, LAST_ONLINE from USERS where " + field + " = ?");
    query.addBindValue(value);
    query.exec();

    if (!query.next()) {
        return NoUser;
    }
    UserRecord urec = {
        query.value(0).toUInt(), query.value(1).toString(), query.value(2).toByteArray(),
        query.value(3).toByteArray(), query.value(4).toDate(), query.value(5).toString(),
        (UserRecord::Flags)query.value(6).toUInt(), QChar(query.value(7).toUInt()),
        query.value(8).toDateTime(), query.value(9).toDateTime() };
    return urec;
}

/**
 * Helper function for logon validation methods.
 */
static void validateLogon (const UserRecord& urec, const Callback& callback)
{
    // check the flags; make sure they're not banned
    if (urec.flags.testFlag(UserRecord::Banned)) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(UserRepository::Banned)));
        return;
    }

    // update the last online timestamp
    QSqlQuery query;
    query.prepare("update USERS set LAST_ONLINE = ? where ID = ?");
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(urec.id);
    query.exec();

    // report success
    callback.invoke(Q_ARG(const QVariant&, QVariant(userRecordType, &urec)));
}

void UserRepository::validateLogon (
    const QString& name, const QString& password, const Callback& callback)
{
    UserRecord urec = loadUserRecord("NAME_LOWER", name.toLower());
    if (urec.id == 0) {
        // no such user
        callback.invoke(Q_ARG(const QVariant&, QVariant(NoSuchUser)));
        return;
    }

    // validate the password hash
    if (hashPassword(password, urec.passwordSalt) != urec.passwordHash) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(WrongPassword)));
        return;
    }

    ::validateLogon(urec, callback);
}

void UserRepository::loadUser (const QString& name, const Callback& callback)
{
    callback.invoke(Q_ARG(const UserRecord&, loadUserRecord("NAME_LOWER", name.toLower())));
}

void UserRepository::usernameForEmail (const QString& email, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("select NAME from USERS where EMAIL = ?");
    query.addBindValue(email.toLower());
    query.exec();

    callback.invoke(Q_ARG(const QString&, query.next() ? query.value(0).toString() : ""));
}

UserRecord UserRepository::loadUser (quint32 id)
{
    return loadUserRecord("ID", id);
}

void UserRepository::updateUser (const UserRecord& urec, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("update USERS set NAME = ?, NAME_LOWER = ?, PASSWORD_HASH = ?, "
        "DATE_OF_BIRTH = ?, EMAIL = ?, FLAGS = ?, AVATAR = ? where ID = ?");
    query.addBindValue(urec.name);
    query.addBindValue(urec.name.toLower());
    query.addBindValue(urec.passwordHash);
    query.addBindValue(urec.dateOfBirth);
    query.addBindValue(urec.email.toLower());
    query.addBindValue((int)urec.flags);
    query.addBindValue(urec.avatar.unicode());
    query.addBindValue(urec.id);

    callback.invoke(Q_ARG(bool, query.exec()));
}

void UserRepository::deleteUser (quint32 id)
{
    QSqlQuery query;
    query.prepare("delete from USERS where ID = ?");
    query.addBindValue(id);
    query.exec();
}

void UserRepository::insertPasswordReset (quint32 userId, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("insert into PASSWORD_RESETS (TOKEN, USER_ID, CREATED) values (?, ?, ?)");
    QByteArray token = generateToken(16);
    query.addBindValue(token);
    query.addBindValue(userId);
    query.addBindValue(QDateTime::currentDateTime());
    query.exec();

    callback.invoke(Q_ARG(quint32, query.lastInsertId().toUInt()),
        Q_ARG(const QByteArray&, token));
}

void UserRepository::validatePasswordReset (
    quint32 id, const QByteArray& token, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("select USER_ID from PASSWORD_RESETS where ID = ? and TOKEN = ?");
    query.addBindValue(id);
    query.addBindValue(token);
    query.exec();

    // make sure it exists
    if (!query.next()) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(NoSuchUser)));
        return;
    }
    quint32 userId = query.value(0).toUInt();

    // delete it, making sure that no one beat us to the punch
    query.prepare("delete from PASSWORD_RESETS where ID = ?");
    query.addBindValue(id);
    query.exec();
    if (query.numRowsAffected() == 0) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(NoSuchUser)));
        return;
    }
    ::validateLogon(loadUser(userId), callback);
}

void UserRecord::setPassword (const QString& password)
{
    passwordHash = hashPassword(password, passwordSalt);
}
