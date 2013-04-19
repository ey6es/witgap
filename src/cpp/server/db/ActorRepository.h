//
// $Id$

#ifndef ACTOR_REPOSITORY
#define ACTOR_REPOSITORY

#include <QObject>

class Callback;

/**
 * Handles database queries associated with actors.
 */
class ActorRepository : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();
    
    /**
     * Finds actors whose names start with the specified prefix.  The callback will receive a
     * ResourceDescriptorList containing the actor descriptors.
     *
     * @param creatorId the id of the creator whose actors are desired, or 0 for all creators.
     */
    Q_INVOKABLE void findActors (
        const QString& prefix, quint32 creatorId, const Callback& callback);
};

#endif // ACTOR_REPOSITORY
