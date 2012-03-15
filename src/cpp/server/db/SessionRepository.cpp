//
// $Id$

#include <QDateTime>
#include <QSqlQuery>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SessionRepository.h"
#include "db/UserRepository.h"
#include "util/Callback.h"

SessionRepository::SessionRepository (ServerApp* app) :
    _app(app)
{
}

void SessionRepository::init ()
{
    // create the table if it doesn't yet exist
    QSqlQuery query;
    query.exec(
        "create table if not exists SESSIONS ("
            "ID bigint unsigned not null auto_increment primary key,"
            "TOKEN binary(16) not null,"
            "USER_ID int unsigned not null default 0,"
            "LAST_ONLINE datetime not null,"
            "index (LAST_ONLINE),"
            "index (USER_ID)");
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
            query.prepare("select USER_ID from SESSIONS where ID = ?");
            query.addBindValue(id);
            query.exec();
            quint32 userId = query.next() ? query.value(0).toUInt() : 0;
            callback.invoke(Q_ARG(quint64, id), Q_ARG(const QByteArray&, token),
                Q_ARG(const UserRecord&, (userId == 0) ? NoUser :
                    _app->databaseThread()->userRepository()->loadUser(userId)));
            return;
        }
    }

    // if that didn't work, we must generate a new id and token
    QByteArray ntoken(16, 0);
    for (int ii = 0; ii < 16; ii++) {
        ntoken[ii] = qrand() % 256;
    }
    query.prepare("insert into SESSIONS (TOKEN, LAST_ONLINE) values (?, ?)");
    query.addBindValue(ntoken);
    query.addBindValue(now);
    query.exec();
    id = query.lastInsertId().toULongLong();
    callback.invoke(Q_ARG(quint64, id), Q_ARG(const QByteArray&, ntoken),
        Q_ARG(const UserRecord&, NoUser));
}

void SessionRepository::setUserId (quint64 id, quint32 userId)
{
    QSqlQuery query;
    query.prepare("update SESSIONS set USER_ID = ? where ID = ?");
    query.addBindValue(userId);
    query.addBindValue(id);
    query.exec();
}
