//
// $Id$

#include <QCryptographicHash>
#include <QSqlQuery>
#include <QtDebug>

#include "db/UserRepository.h"
#include "util/Callback.h"

// register our types with the metatype system
int userRecordType = qRegisterMetaType<UserRecord>();

void UserRepository::init ()
{
    // create the table if it doesn't yet exist
    QSqlQuery query;
    query.exec(
        "create table if not exists USERS ("
            "ID int unsigned not null auto_increment primary key,"
            "NAME varchar(16) not null,"
            "NAME_LOWER varchar(16) not null unique,"
            "PASSWORD_HASH binary(16) not null,"
            "PASSWORD_SALT binary(8) not null,"
            "DATE_OF_BIRTH date not null,"
            "EMAIL varchar(255) not null,"
            "FLAGS int unsigned not null,"
            "CREATED datetime not null,"
            "LAST_ONLINE datetime not null)");
}

void UserRepository::insertUser (
    const QString& name, const QString& password, const QDate& dob,
    const QString& email, const Callback& callback)
{
    QSqlQuery query;
    QDateTime now = QDateTime::currentDateTime();

    // generate a random salt
    QByteArray salt(8, 0);
    for (int ii = 0; ii < 8; ii++) {
        salt[ii] = qrand() % 256;
    }

    // use it to hash the password
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(password.toUtf8());
    hash.addData(salt);

    query.prepare(
        "insert into USERS (NAME, NAME_LOWER, PASSWORD_HASH, PASSWORD_SALT, DATE_OF_BIRTH, "
        "EMAIL, FLAGS, CREATED, LAST_ONLINE) values (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(name.toLower());
    QByteArray hashResult = hash.result();
    query.addBindValue(hashResult);
    query.addBindValue(salt);
    query.addBindValue(dob);
    query.addBindValue(email);
    query.addBindValue(0);
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        callback.invoke(Q_ARG(const UserRecord&, NoUser));
        return;
    }
    UserRecord urec = {
        query.lastInsertId().toUInt(), name, hashResult, salt, dob, email, 0, now, now };
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
        "select ID, NAME, PASSWORD_HASH, PASSWORD_SALT, DATE_OF_BIRTH, EMAIL, FLAGS, CREATED, "
        "LAST_ONLINE from USERS where " + field + " = ?");
    query.addBindValue(value);
    query.exec();

    if (!query.next()) {
        return NoUser;
    }
    UserRecord urec = {
        query.value(0).toUInt(), query.value(1).toString(), query.value(2).toByteArray(),
        query.value(3).toByteArray(), query.value(4).toDate(), query.value(5).toString(),
        (UserRecord::Flags)query.value(6).toUInt(), query.value(7).toDateTime(),
        query.value(8).toDateTime() };
    return urec;
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
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(password.toUtf8());
    hash.addData(urec.passwordSalt);
    if (hash.result() != urec.passwordHash) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(WrongPassword)));
        return;
    }

    // check the flags; make sure they're not banned
    if (urec.flags.testFlag(UserRecord::Banned)) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(Banned)));
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

void UserRepository::loadUser (const QString& name, const Callback& callback)
{
    callback.invoke(Q_ARG(const UserRecord&, loadUserRecord("NAME_LOWER", name.toLower())));
}

UserRecord UserRepository::loadUser (quint32 id)
{
    return loadUserRecord("ID", id);
}
