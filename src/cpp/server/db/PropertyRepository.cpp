//
// $Id$

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/PropertyRepository.h"
#include "peer/PeerManager.h"
#include "util/General.h"

PropertyRepository::PropertyRepository (ServerApp* app) :
    _app(app)
{
}

void PropertyRepository::registerPersistentObject (QObject* object, const QString& name)
{
    // assign the name, if given one
    if (!name.isEmpty()) {
        object->setObjectName(name);
    }

    // create persisters for each property
    const QMetaObject* meta = object->metaObject();
    for (int ii = 0, nn = meta->propertyCount(); ii < nn; ii++) {
        const QMetaProperty& property = meta->property(ii);
        if (property.hasNotifySignal()) {
            new PropertyPersister(_app, object, property);
        }
    }
}

void PropertyRepository::init ()
{
    // create the table if it doesn't yet exist
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery query;

    if (!database.tables().contains("PROPERTIES")) {
        qDebug() << "Creating PROPERTIES table.";
        query.exec(
            "create table PROPERTIES ("
                "OBJECT_NAME varchar(255) not null,"
                "PROPERTY_NAME varchar(255) not null,"
                "VALUE blob not null,"
                "primary key (OBJECT_NAME, PROPERTY_NAME))");
    }
}

void PropertyRepository::loadProperty (
    const QString& objectName, const QString& propertyName, const Callback& callback)
{
    QSqlQuery query;
    query.prepare("select VALUE from PROPERTIES where OBJECT_NAME = ? and PROPERTY_NAME = ?");
    query.addBindValue(objectName);
    query.addBindValue(propertyName);
    query.exec();

    QVariant value;
    if (query.next()) {
        QByteArray data = query.value(0).toByteArray();
        QDataStream in(data);
        in >> value;
    }
    callback.invoke(Q_ARG(const QVariant&, value));
}

void PropertyRepository::storeProperty (
    const QString& objectName, const QString& propertyName, const QVariant& value)
{
    // stream the value to a byte array
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << value;

    // first try updating
    QSqlQuery query;
    query.prepare("update PROPERTIES set VALUE = ? where OBJECT_NAME = ? and PROPERTY_NAME = ?");
    query.addBindValue(data);
    query.addBindValue(objectName);
    query.addBindValue(propertyName);
    query.exec();

    // if that fails, insert
    if (query.numRowsAffected() > 0) {
        return;
    }
    query.prepare("insert into PROPERTIES (OBJECT_NAME, PROPERTY_NAME, VALUE) values (?, ?, ?)");
    query.addBindValue(objectName);
    query.addBindValue(propertyName);
    query.addBindValue(data);
    query.exec();
}

PropertyPersister* PropertyPersister::instance (QObject* object, const QMetaProperty& property)
{
    return static_cast<PropertyPersister*>(object->property(name(property)).value<QObject*>());
}

QByteArray PropertyPersister::name (const QMetaProperty& property)
{
    return QByteArray(property.name()) += "Persister";
}

PropertyPersister::PropertyPersister (
        ServerApp* app, QObject* object, const QMetaProperty& property) :
    CallableObject(object),
    _app(app),
    _property(property),
    _ignoreChange(false)
{
    object->setProperty(name(property), QVariant::fromValue<QObject*>(this));
    connect(object, signal(_property.notifySignal().signature()), SLOT(propertyChanged()));

    // load the initial value
    QMetaObject::invokeMethod(_app->databaseThread()->propertyRepository(), "loadProperty",
        Q_ARG(const QString&, object->objectName()), Q_ARG(const QString&, _property.name()),
        Q_ARG(const Callback&, Callback(_this, "setProperty(QVariant)")));
}

PropertyPersister::~PropertyPersister ()
{
    parent()->setProperty(name(_property), QVariant());
}

void PropertyPersister::propertyChanged ()
{
    if (_ignoreChange) {
        return;
    }
    QObject* parent = this->parent();
    QMetaObject::invokeMethod(_app->databaseThread()->propertyRepository(), "storeProperty",
        Q_ARG(const QString&, parent->objectName()), Q_ARG(const QString&, _property.name()),
        Q_ARG(const QVariant&, _property.read(parent)));
}

void PropertyPersister::setProperty (const QVariant& value)
{
    if (value.isValid()) {
        // ignore changes here and in the transmitter, if any
        setIgnoreChange(true);
        QObject* parent = this->parent();
        PropertyTransmitter* transmitter = PropertyTransmitter::instance(parent, _property);
        if (transmitter != 0) {
            transmitter->setIgnoreChange(true);
        }
        _property.write(parent, value);
        if (transmitter != 0) {
            transmitter->setIgnoreChange(false);
        }
        setIgnoreChange(false);
    }
}
