//
// $Id$

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

#include "db/ActorRepository.h"
#include "util/Callback.h"

void ActorRepository::init ()
{
    // create the tables if they don't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("ACTORS")) {
        /*
        qDebug() << "Creating ACTORS table.";
        query.exec(
            "create table ACTORS ("
                "ID int unsigned not null auto_increment primary key,"
                "NAME varchar(255) not null,"
                "NAME_LOWER varchar(255) not null,"
                "CREATOR_ID bigint unsigned not null,"
                "CREATED datetime not null,"
                "CLASS varchar(255) not null,"
                "CHARACTER int not null,"
                "LABEL varchar(255) not null,"
                "COLLISION_FLAGS int not null,"
                "COLLISION_MASK int not null,"
                "SPAWN_MASK int not null,"
                "index (NAME_LOWER),"
                "index (CREATOR_ID))");
        */
    }
}

void ActorRepository::findActors (
    const QString& prefix, quint64 creatorId, const Callback& callback)
{
}
