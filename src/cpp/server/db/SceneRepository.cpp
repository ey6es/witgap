//
// $Id$

#include <string.h>

#include <QSqlQuery>

#include "db/SceneRepository.h"
#include "util/Callback.h"

// register our types with the metatype system
int sceneRecordType = qRegisterMetaType<SceneRecord>();

void SceneRepository::init ()
{
    // create the tables if they don't yet exist
    QSqlQuery query;
    query.exec(
        "create table if not exists SCENES ("
            "ID int unsigned not null auto_increment primary key,"
            "NAME varchar(255) not null,"
            "CREATOR_ID int unsigned not null,"
            "CREATED datetime not null,"
            "SCROLL_WIDTH smallint unsigned not null,"
            "SCROLL_HEIGHT smallint unsigned not null,"
            "index (CREATOR_ID))");

    query.exec(
        "create table if not exists SCENE_DATA ("
            "SCENE_ID int unsigned not null,"
            "X int not null,"
            "Y int not null,"
            "DATA binary(4096) not null,"
            "index (SCENE_ID),"
            "index (X, Y))");
}

void SceneRepository::insertScene (
    const QString& name, quint32 creatorId, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("insert into SCENES (NAME, CREATOR_ID, CREATED, SCROLL_WIDTH, SCROLL_HEIGHT) "
        "values (?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(creatorId);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(100);
    query.addBindValue(100);
    query.exec();

    callback.invoke(Q_ARG(quint32, query.lastInsertId().toUInt()));
}

void SceneRepository::loadScene (quint32 id, const Callback& callback)
{
    QSqlQuery query;
    query.prepare(
        "select NAME, CREATOR_ID, CREATED, SCROLL_WIDTH, SCROLL_HEIGHT from SCENES where ID = ?");
    query.addBindValue(id);
    query.exec();

    if (!query.next()) {
        callback.invoke(Q_ARG(const SceneRecord&, NoScene));
        return;
    }
    SceneRecord scene = {
        id, query.value(0).toString(), query.value(1).toUInt(), query.value(2).toDateTime(),
        query.value(3).toUInt(), query.value(4).toUInt() };

    query.prepare("select X, Y, DATA from SCENE_DATA where SCENE_ID = ?");
    query.addBindValue(id);
    query.exec();

    while (query.next()) {
        QByteArray bdata = query.value(2).toByteArray();
        QIntVector idata(bdata.length() / sizeof(int));
        memcpy(idata.data(), bdata.constData(), bdata.length());
        scene.data.insert(QPoint(query.value(0).toInt(), query.value(1).toInt()), idata);
    }

    callback.invoke(Q_ARG(const SceneRecord&, scene));
}
