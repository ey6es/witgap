//
// $Id$

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

#include "db/PeerRepository.h"
#include "util/Callback.h"

// register our types with the metatype system
int peerRecordType = qRegisterMetaType<PeerRecord>();
int peerRecordListType = qRegisterMetaType<PeerRecordList>("PeerRecordList");

void PeerRepository::init ()
{
    // create the tables if they don't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("PEERS")) {
        qDebug() << "Creating PEERS table.";
        query.exec(
            "create table PEERS ("
                "NAME varchar(255) not null unique,"
                "REGION varchar(255) not null,"
                "INTERNAL_HOSTNAME varchar(255) not null,"
                "EXTERNAL_HOSTNAME varchar(255) not null,"
                "PORT smallint unsigned not null,"
                "ACTIVE boolean not null,"
                "UPDATED datetime not null)");
    }
}

void PeerRepository::loadPeers (const Callback& callback)
{
    QSqlQuery query;
    query.prepare("select NAME, REGION, INTERNAL_HOSTNAME, EXTERNAL_HOSTNAME, PORT, ACTIVE, "
        "UPDATED from PEERS");
    query.exec();

    PeerRecordList peers;
    while (query.next()) {
        PeerRecord peer = { query.value(0).toString(), query.value(1).toString(),
            query.value(2).toString(), query.value(3).toString(), query.value(4).toUInt(),
            query.value(5).toBool(), query.value(6).toDateTime() };
        peers.append(peer);
    }
    callback.invoke(Q_ARG(const PeerRecordList&, peers));
}

void PeerRepository::storePeer (const PeerRecord& prec)
{
    // first try updating
    QSqlQuery query;
    query.prepare("update PEERS set REGION = ?, INTERNAL_HOSTNAME = ?, EXTERNAL_HOSTNAME = ?, "
        "PORT = ?, ACTIVE = ?, UPDATED = ? where NAME = ?");
    query.addBindValue(prec.region);
    query.addBindValue(prec.internalHostname);
    query.addBindValue(prec.externalHostname);
    query.addBindValue(prec.port);
    query.addBindValue(prec.active);
    QDateTime now = QDateTime::currentDateTime();
    query.addBindValue(now);
    query.addBindValue(prec.name);
    query.exec();

    // if that didn't work, insert
    if (query.numRowsAffected() > 0) {
        return;
    }
    query.prepare("insert into PEERS (NAME, REGION, INTERNAL_HOSTNAME, EXTERNAL_HOSTNAME, PORT, "
        "ACTIVE, UPDATED) values (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(prec.name);
    query.addBindValue(prec.region);
    query.addBindValue(prec.internalHostname);
    query.addBindValue(prec.externalHostname);
    query.addBindValue(prec.port);
    query.addBindValue(prec.active);
    query.addBindValue(now);
    query.exec();
}
