//
// $Id$

#ifndef RUNTIME_CONFIG
#define RUNTIME_CONFIG

#include <QObject>

#include "peer/PeerManager.h"

/**
 * Contains the runtime configurable properties of the server.
 */
class RuntimeConfig : public QObject, public SharedObject
{
    Q_OBJECT
    Q_PROPERTY(LogonPolicy logonPolicy READ logonPolicy WRITE setLogonPolicy
        NOTIFY logonPolicyChanged)
    Q_PROPERTY(quint32 introZone READ introZone WRITE setIntroZone NOTIFY introZoneChanged)
    Q_PROPERTY(quint32 introScene READ introScene WRITE setIntroScene NOTIFY introSceneChanged)
    Q_ENUMS(LogonPolicy)

public:

    /** The available logon policies. */
    enum LogonPolicy { AdminsOnly, InsidersOnly, Everyone };

    /**
     * Initializes the runtime configuration.
     */
    RuntimeConfig (ServerApp* app);

    /**
     * Initializes a copy of the configuration.
     */
    RuntimeConfig (QObject* object = 0);

    /**
     * Sets the logon policy.
     */
    void setLogonPolicy (LogonPolicy policy);

    /**
     * Returns the current logon policy.
     */
    LogonPolicy logonPolicy () const { return _logonPolicy; }

    /**
     * Sets the intro zone.
     */
    void setIntroZone (quint32 id);

    /**
     * Returns the current intro zone.
     */
    quint32 introZone () const { return _introZone; }

    /**
     * Sets the intro scene.
     */
    void setIntroScene (quint32 id);

    /**
     * Returns the current intro scene.
     */
    quint32 introScene () const { return _introScene; }

signals:

    /**
     * Fired when the logon policy changes.
     */
    void logonPolicyChanged (RuntimeConfig::LogonPolicy policy);

    /**
     * Fired when the intro zone changes.
     */
    void introZoneChanged (quint32 id);

    /**
     * Fired when the intro scene changes.
     */
    void introSceneChanged (quint32 id);

protected:

    /** The current logon policy. */
    LogonPolicy _logonPolicy;

    /** The intro zone. */
    quint32 _introZone;

    /** The intro scene. */
    quint32 _introScene;
};

#endif // RUNTIME_CONFIG
