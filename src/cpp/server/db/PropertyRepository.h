//
// $Id$

#ifndef PROPERTY_REPOSITORY
#define PROPERTY_REPOSITORY

#include <QMetaProperty>
#include <QObject>

#include "util/Callback.h"

class ServerApp;

/**
 * Handles database queries associated with persistent object properties.
 */
class PropertyRepository : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates the property repository.
     */
    PropertyRepository (ServerApp* app);

    /**
     * Registers the specified object as needing to have its properties persisted.  This should be
     * called from the object's thread.
     *
     * @param name a name which, if not empty, will assigned to the object.
     */
    void registerPersistentObject (QObject* object, const QString& name = QString());

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Loads a property value.  The callback will receive the value as a QVariant (invalid if the
     * property wasn't found).
     */
    Q_INVOKABLE void loadProperty (
        const QString& objectName, const QString& propertyName, const Callback& callback);

    /**
     * Stores a property value.
     */
    Q_INVOKABLE void storeProperty (
        const QString& objectName, const QString& propertyName, const QVariant& value);

protected:

    /** The server application. */
    ServerApp* _app;
};

/**
 * Listens for property changes in order to persist them in the repository.
 */
class PropertyPersister : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Initializes the persister.
     */
    PropertyPersister (ServerApp* app, QObject* object, const QMetaProperty& property);

public slots:

    /**
     * Called when the tracked property has changed.
     */
    void propertyChanged ();

protected:

    /**
     * Sets the value to one loaded from the database.
     */
    Q_INVOKABLE void setProperty (const QVariant& value);

    /** The application object. */
    ServerApp* _app;

    /** The property that we watch. */
    QMetaProperty _property;
};

#endif // PROPERTY_REPOSITORY
