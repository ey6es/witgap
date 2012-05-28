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
    Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)

public:

    /**
     * Initializes the runtime configuration.
     */
    RuntimeConfig (ServerApp* app);

    /**
     * Initializes a copy of the configuration.
     */
    RuntimeConfig (QObject* object);

    /**
     * Sets whether or not we're open to the public.
     */
    void setOpen (bool open);

    /**
     * Returns whether or not we're open to the public.
     */
    bool open () const { return _open; }

signals:

    /**
     * Fired when the open state changes.
     */
    void openChanged (bool open);

protected:

    /** Whether or not we're open to the public. */
    bool _open;
};

#endif // RUNTIME_CONFIG
