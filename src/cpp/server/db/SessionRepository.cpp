//
// $Id$

#include <time.h>

#include <QDateTime>
#include <QSqlQuery>
#include <QtDebug>

#include "db/SessionRepository.h"

void SessionRepository::init ()
{
    // create the table if it doesn't yet exist
    QSqlQuery query;
    query.exec(
        "create table if not exists SESSIONS ("
            "ID bigint unsigned not null auto_increment primary key,"
            "TOKEN binary(16) not null,"
            "LAST_ONLINE timestamp not null,"
            "index (LAST_ONLINE))");

    // seed the random number generator
    qsrand(clock());
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
            callback.invoke(Q_ARG(quint64, id), Q_ARG(const QByteArray&, token));
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
    callback.invoke(Q_ARG(quint64, id), Q_ARG(const QByteArray&, ntoken));
}
