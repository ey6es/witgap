//
// $Id$

#include <QCryptographicHash>
#include <QDateTime>
#include <QSqlQuery>

#include "db/UserRepository.h"

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
            "CREATED timestamp not null,"
            "LAST_ONLINE timestamp not null)");
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
    query.addBindValue(hash.result());
    query.addBindValue(salt);
    query.addBindValue(dob);
    query.addBindValue(email);
    query.addBindValue(0);
    query.addBindValue(now);
    query.addBindValue(now);

    // if it works, return the id
    callback.invoke(Q_ARG(quint32, query.exec() ? query.lastInsertId().toUInt() : 0));
}

void UserRepository::validateLogon (
    const QString& name, const QString& password, const Callback& callback)
{
    QSqlQuery query;

    // look up the user id, password hash and salt
    query.prepare(
        "select ID, NAME, PASSWORD_HASH, PASSWORD_SALT, FLAGS from USERS where NAME_LOWER = ?");
    query.addBindValue(name.toLower());
    query.exec();

    if (!query.next()) {
        // no such user
        return;
    }

    // validate the password hash
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(password.toUtf8());
    hash.addData(query.value(3).toByteArray());
    if (hash.result() != query.value(2)) {
        // wrong password
        return;
    }

    // check the flags; make sure they're not banned
    quint32 flags = query.value(4).toUInt();

    // get the other bits
    quint32 id = query.value(0).toUInt();
    QString casedName = query.value(1).toString();

    // update the last online timestamp
    query.prepare("update USERS set LAST_ONLINE = ? where ID = ?");
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(id);
    query.exec();

    // report success

}
