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
class Zone : public CallableObject
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
     * Returns a reference to the instance map.
     */
    const QHash<quint32, Instance*>& instances () const { return _instances; }

    /**
     * Creates a new instance of this zone for the identified session.  The callback will
     * receive the instance id.
     */
    void createInstance (quint64 sessionId, const Callback& callback);

protected:

    /**
     * Continues the process of creating a new instance.
     */
    Q_INVOKABLE void continueCreatingInstance (
        quint64 sessionId, const Callback& callback, quint64 instanceId);

    /** The application object. */
    ServerApp* _app;

    /** The zone record. */
    ZoneRecord _record;

    /** The active instances, mapped by offset. */
    QHash<quint32, Instance*> _instances;
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
    Instance (Zone* zone, quint64 id);

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

/**
 * Creates an instance id from the supplied zone id and offset.
 */
inline quint64 createInstanceId (quint32 zoneId, quint32 instanceOffset)
{
    return (quint64)zoneId << 32 | instanceOffset;
}

/**
 * Extracts the zone id from the given instance id.
 */
inline quint32 getZoneId (quint64 instanceId)
{
    return instanceId >> 32;
}

/**
 * Extracts the instance offset from the given instance id.
 */
inline quint32 getInstanceOffset (quint64 instanceId)
{
    return instanceId;
}

#endif // ZONE
