//
// $Id$

#ifndef SCENE_MANAGER
#define SCENE_MANAGER

#include <QHash>
#include <QList>
#include <QObject>
#include <QVector>

#include "peer/PeerManager.h"
#include "util/Callback.h"

class QThread;

class Instance;
class Scene;
class SceneRecord;
class ServerApp;
class Zone;
class ZoneRecord;

/**
 * Manages the set of loaded scenes.
 */
class SceneManager : public CallableObject, public SharedObject
{
    Q_OBJECT

public:

    /**
     * Initializes the scene manager.
     */
    SceneManager (ServerApp* app);

    /**
     * Returns the instance with the provided id, or 0 if not found.
     */
    Instance* instance (quint64 id) const;

    /**
     * Starts the scene threads.
     */
    void startThreads ();

    /**
     * Stops the scene threads.
     */
    void stopThreads ();

    /**
     * Returns the next thread in the round-robin sequence.
     */
    QThread* nextThread ();

    /**
     * Creates a new instance of the identified zone for the identified session.  The callback will
     * receive the instance id.
     */
    Q_INVOKABLE void createInstance (quint64 sessionId, quint32 zoneId, const Callback& callback);

    /**
     * Attempts to reserve a place in the identified instance for the identified session.  The
     * callback will receive a bool indicating success or failure.
     */
    Q_INVOKABLE void reserveInstancePlace (
        quint64 sessionId, quint64 instanceId, const Callback& callback);

    /**
     * Cancels an instance place reservation that we've decided we no longer need.
     */
    Q_INVOKABLE void cancelInstancePlaceReservation (quint64 sessionId, quint64 instanceId);

    /**
     * Attempts to resolve a zone.  The callback will receive a QObject*, either the resolved zone
     * or 0 if not found.
     */
    Q_INVOKABLE void resolveZone (quint32 id, const Callback& callback);

    /**
     * Broadcasts a change of zone record to instances on all peers.
     */
    Q_INVOKABLE void broadcastZoneRecordUpdated (const ZoneRecord& record);

    /**
     * Notifies the manager that a zone record has been modified in the database.
     */
    Q_INVOKABLE void zoneRecordUpdated (const ZoneRecord& record);

protected:

    /**
     * Called when a request to load a zone returns.
     */
    Q_INVOKABLE void zoneMaybeLoaded (quint32 id, const ZoneRecord& record);

    /**
     * Continues the process of creating an instance, having resolved the zone.
     */
    Q_INVOKABLE void continueCreatingInstance (
        quint64 sessionId, const Callback& callback, QObject* zobj);

    /** The application object. */
    ServerApp* _app;

    /** Loaded zones mapped by id. */
    QHash<quint32, Zone*> _zones;

    /** Callbacks awaiting zone resolution. */
    QHash<quint32, QList<Callback> > _zonePenders;

    /** The list of scene threads. */
    QVector<QThread*> _threads;

    /** The index of the last thread to which we assigned a scene. */
    int _lastThreadIdx;
};

#endif // SCENE_MANAGER
