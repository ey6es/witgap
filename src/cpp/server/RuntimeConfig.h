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
    Q_PROPERTY(Flags flags READ flags WRITE setFlags NOTIFY flagsChanged)
    Q_FLAGS(Flag Flags)

public:

    /** The configuration flags. */
    enum Flag {
        AllowLogon = 0x01,
        AllowCreate = 0x02
    };
    Q_DECLARE_FLAGS(Flags, Flag)

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

    /**
     * Sets the set of flags.
     */
    void setFlags (Flags flags);

    /**
     * Returns the set of flags.
     */
    Flags flags () const { return _flags; }

signals:

    /**
     * Fired when the open state changes.
     */
    void openChanged (bool open);

    /**
     * Fired when the flags change.
     */
    void flagsChanged (Flags flags);

protected:

    /** Whether or not we're open to the public. */
    bool _open;

    /** The set of flags. */
    Flags _flags;
};

#endif // RUNTIME_CONFIG
