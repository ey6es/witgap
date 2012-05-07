//
// $Id$

#ifndef PEER_REPOSITORY
#define PEER_REPOSITORY

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QObject>

class Callback;
class PeerRecord;

/**
 * Handles database queries associated with peers.
 */
class PeerRepository : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Loads all peer records.  The callback will receive a PeerRecordList containing the records.
     */
    Q_INVOKABLE void loadPeers (const Callback& callback);

    /**
     * Inserts or updates a peer record.
     */
    Q_INVOKABLE void storePeer (const PeerRecord& prec);
};

/**
 * The information associated with a single peer.
 */
class PeerRecord
{
public:

    /** The peer's name. */
    QString name;

    /** The peer's geographic region. */
    QString region;

    /** The peer's hostname within the region. */
    QString internalHostname;

    /** The peer's hostname outside of the region. */
    QString externalHostname;

    /** The peer port. */
    quint16 port;

    /** Whether or not the peer is currently running. */
    bool active;

    /** The time at which the peer record was last updated. */
    QDateTime updated;
};

Q_DECLARE_METATYPE(PeerRecord)

/** A list of peer records that we'll register with the metatype system. */
typedef QList<PeerRecord> PeerRecordList;

#endif // PEER_REPOSITORY
