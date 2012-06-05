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

signals:

    /**
     * Fired when the logon policy changes.
     */
    void logonPolicyChanged (RuntimeConfig::LogonPolicy policy);

protected:

    /** The current logon policy. */
    LogonPolicy _logonPolicy;
};

#endif // RUNTIME_CONFIG
