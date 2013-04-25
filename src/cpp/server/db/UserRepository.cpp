//
// $Id$

#include <QCryptographicHash>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/UserRepository.h"
#include "util/Callback.h"
#include "util/General.h"

UserRepository::UserRepository (ServerApp* app) :
    _app(app)
{
    // read the name chain data
    QFile pfile(app->config().value("name_chain").toString());
    pfile.open(QIODevice::ReadOnly);
    QDataStream pin(&pfile);

    // first, the probabilities for each name length
    for (int ii = 0; ii < MaxNameLength - MinNameLength + 1; ii++) {
        pin >> _nameLengths[ii];
    }

    // then, the probabilities that each state will follow each other
    for (int ii = 0; ii < NameChainStates; ii++) {
        for (int jj = 0; jj < NameChainStates; jj++) {
            pin >> _nameChain[ii][jj];
        }
    }

    // load the blocked named list
    QFile bfile(app->config().value("blocked_names").toString());
    bfile.open(QIODevice::ReadOnly | QIODevice::Text);

    QString pattern;
    while (!bfile.atEnd()) {
        QString line = bfile.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (!pattern.isEmpty()) {
            pattern += '|';
        }
        pattern += line;
    }
    _blockedNameExp = QRegExp(pattern);
}

void UserRepository::init ()
{
    // create the tables if they don't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("USERS")) {
        qDebug() << "Creating USERS table.";
        query.exec(
            "create table USERS ("
                "ID bigint unsigned not null auto_increment primary key,"
                "SESSION_TOKEN binary(16) not null,"
                "NAME varchar(16) not null,"
                "NAME_LOWER varchar(16) not null unique,"
                "AVATAR smallint unsigned not null,"
                "CREATED datetime not null,"
                "LAST_ONLINE datetime not null,"
                "PASSWORD_SALT binary(8) not null,"
                "PASSWORD_HASH binary(16),"
                "DATE_OF_BIRTH date not null,"
                "EMAIL varchar(255) not null,"
                "FLAGS int unsigned not null,"
                "LAST_ZONE_ID int unsigned not null,"
                "LAST_SCENE_ID int unsigned not null,"
                "index (EMAIL))");
    }

    if (!database.tables().contains("PASSWORD_RESETS")) {
        qDebug() << "Creating PASSWORD_RESETS table.";
        query.exec(
            "create table PASSWORD_RESETS ("
                "ID int unsigned not null auto_increment primary key,"
                "TOKEN binary(16) not null,"
                "USER_ID bigint unsigned not null,"
                "CREATED datetime not null,"
                "index (CREATED),"
                "index (USER_ID))");
    }
}

/**
 * Helper function for validateToken: returns a random alphanumeric character for use as an
 * avatar.
 */
static QChar randomAvatar ()
{
    int value = qrand() % 62;
    if (value < 26) {
        return 'a' + value;
    } else if (value < 52) {
        return 'A' + (value - 26);
    } else {
        return '0' + (value - 52);
    }
}

void UserRepository::validateSessionToken (
    quint64 userId, const QByteArray& token, const Callback& callback)
{
    QSqlQuery query;
    QDateTime now = QDateTime::currentDateTime();

    // try to update the timestamp if we were passed what looks like a valid id
    if (userId != 0) {
        query.prepare("update USERS set LAST_ONLINE = ? where ID = ? and TOKEN = ?");
        query.addBindValue(now);
        query.addBindValue(userId);
        query.addBindValue(token);
        query.exec();
        if (query.numRowsAffected() > 0) { // valid; return what we were passed
            // look up the user record
            query.prepare("select NAME, AVATAR, CREATED, PASSWORD_SALT, PASSWORD_HASH, "
                "DATE_OF_BIRTH, EMAIL, FLAGS, LAST_ZONE_ID, LAST_SCENE_ID from USERS "
                "where ID = ?");
            query.addBindValue(userId);
            query.exec();
            query.next();
            UserRecord urec = {
                userId, token, query.value(0).toString(), QChar(query.value(1).toUInt()),
                query.value(2).toDateTime(), now, query.value(3).toByteArray(),
                query.value(4).toByteArray(), query.value(5).toDate(), query.value(6).toString(),
                (UserRecord::Flags)query.value(7).toUInt(), query.value(8).toUInt(),
                query.value(9).toUInt() };

            // make sure they can actually log on
            if (validateLogon(urec) == NoError) {
                qDebug() << "Session resumed." << userId << urec.name;
                callback.invoke(Q_ARG(const UserRecord&, urec));
                return;
            }
        }
    }

    // if that didn't work, we must generate a new user with a random name/avatar
    createUser(callback);
}

void UserRepository::createUser (const Callback& callback)
{
    QSqlQuery query;
    QDateTime now = QDateTime::currentDateTime();

    // generate a random salt
    QByteArray salt(8, 0);
    for (int ii = 0; ii < 8; ii++) {
        salt[ii] = qrand() % 256;
    }
    query.prepare("insert into USERS (SESSION_TOKEN, NAME, NAME_LOWER, AVATAR, "
        "CREATED, LAST_ONLINE, PASSWORD_SALT) values (?, ?, ?, ?, ?, ?, ?)");
    QByteArray ntoken = generateToken(16);
    query.addBindValue(ntoken);
    QString name = uniqueRandomName();
    query.addBindValue(name);
    query.addBindValue(name.toLower());
    QChar avatar = randomAvatar();
    query.addBindValue(avatar.unicode());
    query.addBindValue(now);
    query.addBindValue(now);
    query.addBindValue(salt);
    query.exec();

    UserRecord urec = { query.lastInsertId().toULongLong(), ntoken, name, avatar, now, now, salt };
    qDebug() << "User created." << urec.id << urec.name;
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
        "select ID, SESSION_TOKEN, NAME, AVATAR, CREATED, LAST_ONLINE, PASSWORD_SALT, "
        "PASSWORD_HASH, DATE_OF_BIRTH, EMAIL, FLAGS, LAST_ZONE_ID, LAST_SCENE_ID from "
        "USERS where " + field + " = ?");
    query.addBindValue(value);
    query.exec();

    if (!query.next()) {
        return NoUser;
    }
    UserRecord urec = {
        query.value(0).toULongLong(), query.value(1).toByteArray(), query.value(2).toString(),
        QChar(query.value(3).toUInt()), query.value(4).toDateTime(), query.value(5).toDateTime(),
        query.value(6).toByteArray(), query.value(7).toByteArray(), query.value(8).toDate(),
        query.value(9).toString(), (UserRecord::Flags)query.value(10).toUInt(),
        query.value(11).toUInt(), query.value(12).toUInt() };
    return urec;
}

/**
 * Helper function that returns the hash of the plaintext password and the supplied salt.
 */
static QByteArray hashPassword (const QString& password, const QByteArray& salt)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(password.toUtf8());
    hash.addData(salt);
    return hash.result();
}

void UserRepository::validateLogon (
    const UserRecord& orec, const QString& name, const QString& password, const Callback& callback)
{
    UserRecord nrec = loadUserRecord("NAME_LOWER", name.toLower());
    if (nrec.id == 0) {
        // no such user
        callback.invoke(Q_ARG(const QVariant&, QVariant(NoSuchUser)));
        return;
    }

    // validate the password hash
    if (hashPassword(password, nrec.passwordSalt) != nrec.passwordHash) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(WrongPassword)));
        return;
    }

    // pass to shared validation function
    LogonError error = validateLogon(nrec);
    if (error != NoError) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(error)));
        return;
    }

    // perform the logon
    logon(orec, nrec, callback);
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

void UserRepository::updateUser (const UserRecord& urec, const Callback& callback)
{
    // check the name against our block list
    QString nameLower = urec.name.toLower();
    if (nameLower.contains(_blockedNameExp)) {
        callback.invoke(Q_ARG(bool, false));
        return;
    }

    QSqlQuery query;
    query.prepare("update USERS set NAME = ?, NAME_LOWER = ?, AVATAR = ?, PASSWORD_HASH = ?, "
        "DATE_OF_BIRTH = ?, EMAIL = ?, FLAGS = ? where ID = ?");
    query.addBindValue(urec.name);
    query.addBindValue(nameLower);
    query.addBindValue(urec.avatar.unicode());
    query.addBindValue(urec.passwordHash);
    query.addBindValue(urec.dateOfBirth);
    query.addBindValue(urec.email.toLower());
    query.addBindValue((int)urec.flags);
    query.addBindValue(urec.id);

    callback.invoke(Q_ARG(bool, query.exec()));
}

void UserRepository::deleteUser (quint64 id)
{
    QSqlQuery query;
    query.prepare("delete from USERS where ID = ?");
    query.addBindValue(id);
    query.exec();
}

void UserRepository::insertPasswordReset (quint64 userId, const Callback& callback)
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
    const UserRecord& orec, quint32 id, const QByteArray& token, const Callback& callback)
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
    quint64 userId = query.value(0).toULongLong();

    // if successful, delete it, making sure that no one beat us to the punch
    UserRecord nrec = loadUserRecord("ID", userId);
    LogonError error = validateLogon(nrec);
    if (error != NoError) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(error)));
        return;
    }
    query.prepare("delete from PASSWORD_RESETS where ID = ?");
    query.addBindValue(id);
    query.exec();
    if (query.numRowsAffected() == 0) {
        callback.invoke(Q_ARG(const QVariant&, QVariant(NoSuchUser)));
        return;
    }

    // perform the logon
    logon(orec, nrec, callback);
}

QString UserRepository::uniqueRandomName () const
{
    QSqlQuery query;

    for (int ii = 0;; ii++) {
        // generate a list of possible names
        QStringList names;
        for (int jj = 0; jj < 10; jj++) {
            QString name = randomName();
            if (ii > 1) {
                // start inserting numbers after the first failed iterations
                name.insert(qrand() % (name.length() + 1), QString::number(ii));
            }
            // make sure it isn't in the blocked name list
            if (!name.contains(_blockedNameExp)) {
                names.append(name);
            }
        }

        // remove names used by users
        query.prepare("select NAME_LOWER from USERS where NAME_LOWER in ?");
        query.addBindValue(names);
        query.exec();
        while (query.next()) {
            names.removeAll(query.value(0).toString());
        }

        if (!names.isEmpty()) {
            return names.at(0);
        }
    }
}

QString UserRepository::randomName () const
{
    int length = MinNameLength + randomIndex(_nameLengths);
    QString name(length, ' ');
    int last = 0;
    for (int ii = 0; ii < length; ii++) {
        last = randomIndex(_nameChain[last]);
        name[ii] = 'a' + (last - 1);
    }
    return name;
}

UserRepository::LogonError UserRepository::validateLogon (const UserRecord& urec)
{
    // check the flags; make sure they're not banned
    if (urec.flags.testFlag(UserRecord::Banned)) {
        return Banned;
    }

    // make sure they're allowed on at present
    RuntimeConfig::LogonPolicy policy = _app->databaseThread()->runtimeConfig()->logonPolicy();
    if (policy == RuntimeConfig::AdminsOnly && !urec.flags.testFlag(UserRecord::Admin) ||
           policy == RuntimeConfig::InsidersOnly && !urec.insiderPlus()) {
        return ServerClosed;
    }

    // good to go
    return NoError;
}

void UserRepository::logon (const UserRecord& orec, UserRecord& nrec, const Callback& callback)
{
    // update the session token and last online timestamp
    QSqlQuery query;
    query.prepare("update USERS set SESSION_TOKEN = ?, LAST_ONLINE = ? where ID = ?");
    query.addBindValue(nrec.sessionToken = generateToken(16));
    query.addBindValue(nrec.lastOnline = QDateTime::currentDateTime());
    query.addBindValue(nrec.id);
    query.exec();

    // if the old record has no password, delete it
    if (!orec.loggedOn()) {
        deleteUser(orec.id);
    }

    callback.invoke(Q_ARG(const QVariant&, QVariant::fromValue(nrec)));
}

void UserRecord::setPassword (const QString& password)
{
    passwordHash = hashPassword(password, passwordSalt);
}
