//
// $Id$

#include <string.h>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

#include "db/SceneRepository.h"
#include "util/Callback.h"

// register our types with the metatype system
int sceneRecordType = qRegisterMetaType<SceneRecord>();
int sceneDescriptorListType = qRegisterMetaType<SceneDescriptorList>("SceneDescriptorList");

void SceneRepository::init ()
{
    // create the tables if they don't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("SCENES")) {
        qDebug() << "Creating SCENES table.";
        query.exec(
            "create table SCENES ("
                "ID int unsigned not null auto_increment primary key,"
                "NAME varchar(255) not null,"
                "NAME_LOWER varchar(255) not null,"
                "CREATOR_ID int unsigned not null,"
                "CREATED datetime not null,"
                "SCROLL_WIDTH smallint unsigned not null,"
                "SCROLL_HEIGHT smallint unsigned not null,"
                "index (NAME_LOWER),"
                "index (CREATOR_ID))");
    }

    if (!database.tables().contains("SCENE_BLOCKS")) {
        qDebug() << "Creating SCENE_BLOCKS table.";
        query.exec(
            "create table SCENE_BLOCKS ("
                "SCENE_ID int unsigned not null,"
                "X int not null,"
                "Y int not null,"
                "DATA blob not null,"
                "index (SCENE_ID),"
                "index (X, Y))");
    }
}

void SceneRepository::insertScene (
    const QString& name, quint32 creatorId, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("insert into SCENES (NAME, NAME_LOWER, CREATOR_ID, CREATED, SCROLL_WIDTH, "
        "SCROLL_HEIGHT) values (?, ?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(name.toLower());
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
        "select SCENES.NAME, CREATOR_ID, USERS.NAME, SCENES.CREATED, SCROLL_WIDTH, SCROLL_HEIGHT "
            "from SCENES, USERS where SCENES.CREATOR_ID = USERS.ID and SCENES.ID = ?");
    query.addBindValue(id);
    query.exec();

    if (!query.next()) {
        callback.invoke(Q_ARG(const SceneRecord&, NoScene));
        return;
    }
    SceneRecord scene = {
        id, query.value(0).toString(), query.value(1).toUInt(), query.value(2).toString(),
        query.value(3).toDateTime(), query.value(4).toUInt(), query.value(5).toUInt() };

    query.prepare("select X, Y, DATA from SCENE_BLOCKS where SCENE_ID = ?");
    query.addBindValue(id);
    query.exec();

    while (query.next()) {
        QByteArray data = qUncompress(query.value(2).toByteArray());
        scene.blocks.insert(QPoint(query.value(0).toInt(), query.value(1).toInt()),
            SceneRecord::Block((int*)data.constData()));
    }

    callback.invoke(Q_ARG(const SceneRecord&, scene));
}

void SceneRepository::findScenes (
    const QString& prefix, quint32 creatorId, const Callback& callback)
{
    QSqlQuery query;
    QString escaped = prefix.toLower().replace('%', "\\%").replace('_', "\\_") += '%';
    QString base = "select SCENES.ID, SCENES.NAME, CREATOR_ID, USERS.NAME, SCENES.CREATED from "
        "SCENES, USERS where SCENES.CREATOR_ID = USERS.ID and SCENES.NAME_LOWER like ?";
    if (creatorId == 0) {
        query.prepare(base);
        query.addBindValue(escaped);
    } else {
        query.prepare(base + " and CREATOR_ID = ?");
        query.addBindValue(escaped);
        query.addBindValue(creatorId);
    }
    query.exec();

    SceneDescriptorList descs;
    while (query.next()) {
        SceneDescriptor desc = { query.value(0).toUInt(), query.value(1).toString(),
            query.value(2).toUInt(), query.value(3).toString(), query.value(4).toDateTime() };
        descs.append(desc);
    }
    callback.invoke(Q_ARG(const SceneDescriptorList&, descs));
}

void SceneRepository::updateScene (const SceneRecord& srec)
{
    QSqlQuery query;
    query.prepare("update SCENES set NAME = ?, NAME_LOWER = ?, SCROLL_WIDTH = ?, "
        "SCROLL_HEIGHT = ? where ID = ?");
    query.addBindValue(srec.name);
    query.addBindValue(srec.name.toLower());
    query.addBindValue(srec.scrollWidth);
    query.addBindValue(srec.scrollHeight);
    query.addBindValue(srec.id);
    query.exec();
}

void SceneRepository::updateSceneBlocks (const SceneRecord& srec)
{
    QSqlQuery query;

    query.prepare("insert into SCENE_BLOCKS (SCENE_ID, X, Y, DATA) values (?, ?, ?, ?)");
    foreach (const QPoint& key, srec.added) {
        query.addBindValue(srec.id);
        query.addBindValue(key.x());
        query.addBindValue(key.y());
        query.addBindValue(qCompress((const uchar*)srec.blocks[key].constData(),
            SceneRecord::Block::Size*SceneRecord::Block::Size*sizeof(int)));
        query.exec();
    }

    query.prepare("update SCENE_BLOCKS set DATA = ? where SCENE_ID = ? and X = ? and Y = ?");
    foreach (const QPoint& key, srec.updated) {
        query.addBindValue(qCompress((const uchar*)srec.blocks[key].constData(),
            SceneRecord::Block::Size*SceneRecord::Block::Size*sizeof(int)));
        query.addBindValue(srec.id);
        query.addBindValue(key.x());
        query.addBindValue(key.y());
        query.exec();
    }

    query.prepare("delete from SCENE_BLOCKS where SCENE_ID = ? and X = ? and Y = ?");
    foreach (const QPoint& key, srec.removed) {
        query.addBindValue(srec.id);
        query.addBindValue(key.x());
        query.addBindValue(key.y());
        query.exec();
    }
}

void SceneRepository::deleteScene (quint32 id)
{
    QSqlQuery query;
    query.prepare("delete from SCENES where ID = ?");
    query.addBindValue(id);
    query.exec();

    query.prepare("delete from SCENE_BLOCKS where ID = ?");
    query.addBindValue(id);
    query.exec();
}

SceneRecord::Block::Block () :
    QIntVector(Size*Size, ' '),
    _filled(0)
{
}

SceneRecord::Block::Block (const int* data) :
    QIntVector(Size*Size, ' '),
    _filled(0)
{
    for (int* ptr = this->data(), *end = ptr + Size*Size; ptr < end; ptr++, data++) {
        _filled += ((*ptr = *data) != ' ');
    }
}

void SceneRecord::Block::set (const QPoint& pos, int character)
{
    int& value = (*this)[(pos.y() & Mask) << LgSize | pos.x() & Mask];
    _filled += (value == ' ') - (character == ' ');
    value = character;
}

int SceneRecord::Block::get (const QPoint& pos) const
{
    return at((pos.y() & Mask) << LgSize | pos.x() & Mask);
}

void SceneRecord::set (const QPoint& pos, int character)
{
    QPoint key(pos.x() >> Block::LgSize, pos.y() >> Block::LgSize);
    Block& block = blocks[key];
    int ofilled = block.filled();
    block.set(pos, character);

    // perhaps remove, update delta sets
    if (block.filled() == 0) {
        blocks.remove(key);
        if (ofilled != 0) { // removed
            if (!added.remove(key)) {
                updated.remove(key);
                removed.insert(key);
            }
        }
    } else {
        if (ofilled == 0) { // added
            if (removed.remove(key)) {
                updated.insert(key);
            } else {
                added.insert(key);
            }
        } else { // updated
            if (!added.contains(key)) {
                updated.insert(key);
            }
        }
    }
}

int SceneRecord::get (const QPoint& pos) const
{
    QPoint key(pos.x() >> Block::LgSize, pos.y() >> Block::LgSize);
    QHash<QPoint, Block>::const_iterator it = blocks.constFind(key);
    return (it == blocks.constEnd()) ? ' ' : (*it).get(pos);
}

void SceneRecord::clean ()
{
    added.clear();
    updated.clear();
    removed.clear();
}
