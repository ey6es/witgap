//
// $Id$

#include <QDateTime>
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
                "AVATAR smallint unsigned not null,"
                "USER_ID int unsigned not null default 0,"
                "LAST_ONLINE datetime not null,"
                "index (LAST_ONLINE),"
                "index (USER_ID))");
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
            query.prepare("select AVATAR, USER_ID from SESSIONS where ID = ?");
            query.addBindValue(id);
            query.exec();
            Q_ASSERT(query.next());
            SessionRecord srec = {
                id, token, QChar(query.value(0).toUInt()), query.value(1).toUInt(), now };
            UserRecord urec = (srec.userId == 0) ? NoUser :
                _app->databaseThread()->userRepository()->loadUser(srec.userId);
            qDebug() << "Session resumed." << id << urec.name;
            callback.invoke(Q_ARG(const SessionRecord&, srec), Q_ARG(const UserRecord&, urec));
            return;
        }
    }

    // if that didn't work, we must generate a new id and token and a random avatar
    QByteArray ntoken = generateToken(16);
    QChar avatar = randomAvatar();
    query.prepare("insert into SESSIONS (TOKEN, AVATAR, LAST_ONLINE) values (?, ?, ?)");
    query.addBindValue(ntoken);
    query.addBindValue(avatar.unicode());
    query.addBindValue(now);
    query.exec();

    SessionRecord srec = { query.lastInsertId().toULongLong(), ntoken, avatar, 0, now };
    qDebug() << "Session created." << srec.id;
    callback.invoke(Q_ARG(const SessionRecord&, srec), Q_ARG(const UserRecord&, NoUser));
}

void SessionRepository::updateSession (const SessionRecord& session)
{
    QSqlQuery query;
    query.prepare("update SESSIONS set USER_ID = ?, AVATAR = ? where ID = ?");
    query.addBindValue(session.userId);
    query.addBindValue(session.avatar.unicode());
    query.addBindValue(session.id);
    query.exec();
}
