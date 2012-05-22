//
// $Id$

#ifndef ZONE
#define ZONE

#include <QHash>
#include <QList>
#include <QObject>

#include "db/SceneRepository.h"
#include "peer/PeerManager.h"
#include "util/Callback.h"

class Instance;
class Scene;
class SceneRecord;
class ServerApp;

/**
 * The in-memory representation of a zone.
 */
class Zone : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new zone.
     */
    Zone (ServerApp* app, const ZoneRecord& record);

    /**
     * Returns a pointer to the application object.
     */
    ServerApp* app () const { return _app; }

    /**
     * Returns a reference to the zone record.
     */
    const ZoneRecord& record () const { return _record; }

    /**
     * Reserves a place in an instance of this zone.  The callback will receive a QObject* to the
     * instance.
     */
    Q_INVOKABLE void reserveInstancePlace (const Callback& callback);

protected:

    /** The application object. */
    ServerApp* _app;

    /** The zone record. */
    ZoneRecord _record;

    /** The list of instances with open slots. */
    QList<Instance*> _openInstances;
};

/**
 * A zone instance.
 */
class Instance : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Creates a new instance.
     */
    Instance (Zone* zone);

    /**
     * Returns a pointer to the zone of which this is an instance.
     */
    Zone* zone () const { return _zone; }

    /**
     * Attempts to resolve a scene.  The callback will receive a QObject*, either the resolved
     * scene or 0 if not found.
     */
    void resolveScene (quint32 id, const Callback& callback);

protected:

    /**
     * Called when a request to load a scene returns.
     */
    Q_INVOKABLE void sceneMaybeLoaded (quint32 id, const SceneRecord& record);

    /**
     * Shuts down the instance.
     */
    void shutdown ();

    /** The zone that owns the instance. */
    Zone* _zone;

    /** The instance info. */
    InstanceInfo _info;

    /** Scenes in the instance mapped by id. */
    QHash<quint32, Scene*> _scenes;

    /** Callbacks awaiting scene resolution. */
    QHash<quint32, QList<Callback> > _scenePenders;
};

#endif // ZONE
