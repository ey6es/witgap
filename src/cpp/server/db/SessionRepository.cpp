//
// $Id$

#include <time.h>

#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SessionRepository.h"
#include "db/UserRepository.h"
#include "util/Callback.h"
#include "util/General.h"

// register our types with the metatype system
int sessionRecordType = qRegisterMetaType<SessionRecord>();

SessionRepository::SessionRepository (ServerApp* app) :
    _app(app)
{
    // read the letter probability matrix
    QFile pfile(app->config().value("name_chain").toString());
    pfile.open(QIODevice::ReadOnly);
    QDataStream pin(&pfile);
    for (int ii = 0; ii < MaxNameLength - MinNameLength + 1; ii++) {
        pin >> _nameLengths[ii];
    }
    for (int ii = 0; ii < NameChainStates; ii++) {
        for (int jj = 0; jj < NameChainStates; jj++) {
            pin >> _nameChain[ii][jj];
        }
    }
}

void SessionRepository::init ()
{
    // create the table if it doesn't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("SESSIONS")) {
        qDebug() << "Creating SESSIONS table.";
        query.exec(
            "create table SESSIONS ("
                "ID bigint unsigned not null auto_increment primary key,"
                "TOKEN binary(16) not null,"
                "NAME varchar(16) not null,"
                "AVATAR smallint unsigned not null,"
                "USER_ID int unsigned not null default 0,"
                "LAST_ONLINE datetime not null,"
                "index (LAST_ONLINE),"
                "index (USER_ID),"
                "index (NAME))");
    }
}

/**
 * Helper function for validateToken: returns a random alphanumeric character for use as an
 * avatar.
 */
QChar randomAvatar ()
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

void SessionRepository::validateToken (
    quint64 id, const QByteArray& token, const Callback& callback)
{
    QSqlQuery query;
    QDateTime now = QDateTime::currentDateTime();

    // try to update the timestamp if we were passed what looks like a valid id
    if (id != 0) {
        query.prepare("update SESSIONS set LAST_ONLINE = ? where ID = ? and TOKEN = ?");
        query.addBindValue(now);
        query.addBindValue(id);
        query.addBindValue(token);
        query.exec();
        if (query.numRowsAffected() > 0) { // valid; return what we were passed
            // look up the user record
            query.prepare("select NAME, AVATAR, USER_ID from SESSIONS where ID = ?");
            query.addBindValue(id);
            query.exec();
            Q_ASSERT(query.next());
            SessionRecord srec = {
                id, token, query.value(0).toString(), QChar(query.value(1).toUInt()),
                query.value(2).toUInt(), now };
            UserRecord urec = (srec.userId == 0) ? NoUser :
                _app->databaseThread()->userRepository()->loadUser(srec.userId);
            qDebug() << "Session resumed." << id << srec.name;
            callback.invoke(Q_ARG(const SessionRecord&, srec), Q_ARG(const UserRecord&, urec));
            return;
        }
    }

    // if that didn't work, we must generate a new id and token and a random name/avatar
    QByteArray ntoken = generateToken(16);
    QString name = uniqueRandomName();
    QChar avatar = randomAvatar();
    query.prepare("insert into SESSIONS (TOKEN, NAME, AVATAR, LAST_ONLINE) values (?, ?, ?, ?)");
    query.addBindValue(ntoken);
    query.addBindValue(name);
    query.addBindValue(avatar.unicode());
    query.addBindValue(now);
    query.exec();

    SessionRecord srec = { query.lastInsertId().toULongLong(), ntoken, name, avatar, 0, now };
    qDebug() << "Session created." << srec.id;
    callback.invoke(Q_ARG(const SessionRecord&, srec), Q_ARG(const UserRecord&, NoUser));
}

void SessionRepository::updateSession (const SessionRecord& session)
{
    QSqlQuery query;
    query.prepare("update SESSIONS set USER_ID = ?, NAME = ?, AVATAR = ? where ID = ?");
    query.addBindValue(session.userId);
    query.addBindValue(session.name);
    query.addBindValue(session.avatar.unicode());
    query.addBindValue(session.id);
    query.exec();
}

QString SessionRepository::uniqueRandomName () const
{
    for (int ii = 0;; ii++) {
        // generate a list of possible names
        QStringList names;
        for (int jj = 0; jj < 10; jj++) {
            QString name = randomName();
            if (ii > 1) {
                // start inserting numbers after the first failed iterations
                name.insert(qrand() % (name.length() + 1), QString::number(ii));
            }
            names.append(name);
        }

        // remove names used by sessions
        QSqlQuery query;
        query.prepare("select NAME from SESSIONS where NAME in ?");
        query.addBindValue(names);
        query.exec();
        while (query.next()) {
            names.removeAll(query.value(0).toString());
        }

        // and those used by users
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

QString SessionRepository::randomName () const
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
