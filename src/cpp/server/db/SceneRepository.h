//
// $Id$

#ifndef SCENE_REPOSITORY
#define SCENE_REPOSITORY

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPoint>
#include <QSet>

#include "util/General.h"

class Callback;
class SceneRecord;

/**
 * Handles database queries associated with scenes.
 */
class SceneRepository : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Inserts a new scene into the database.  The callback will receive the (quint32) scene id.
     */
    Q_INVOKABLE void insertScene (
        const QString& name, quint32 creatorId, const Callback& callback);

    /**
     * Loads a scene.  The callback will receive the {@link SceneRecord}.
     */
    Q_INVOKABLE void loadScene (quint32 id, const Callback& callback);

    /**
     * Finds scenes whose names start with the specified prefix.  The callback will receive a
     * SceneDescriptorList containing the scene descriptors.
     *
     * @param creatorId the id of the creator whose scenes are desired, or 0 for all creators.
     */
    Q_INVOKABLE void findScenes (
        const QString& prefix, quint32 creatorId, const Callback& callback);

    /**
     * Updates a scene record.
     */
    Q_INVOKABLE void updateScene (const SceneRecord& srec);

    /**
     * Updates a scene's blocks.
     */
    Q_INVOKABLE void updateSceneBlocks (const SceneRecord& srec);

    /**
     * Deletes the identified scene.
     */
    Q_INVOKABLE void deleteScene (quint32 id);
};

/**
 * Holds the metadata associated with a scene.
 */
class SceneRecord
{
public:

    /**
     * Contains a block of scene data.
     */
    class Block : public QIntVector
    {
    public:

        /** The width/height of each block as a power of two. */
        static const int LgSize = 6;

        /** The width/height of each block. */
        static const int Size = 1 << LgSize;

        /** The coordinate mask. */
        static const int Mask = Size - 1;

        /**
         * Creates an empty block.
         */
        Block ();

        /**
         * Creates a block from the specified data.
         */
        Block (const int* data);

        /**
         * Returns the number of non-empty locations in the block.
         */
        int filled () const { return _filled; }

        /**
         * Sets the value at the specified location.
         */
        void set (const QPoint& pos, int character);

        /**
         * Returns the value at the specified location.
         */
        int get (const QPoint& pos) const;

    protected:

        /** The number of non-empty locations. */
        int _filled;
    };

    /** The scene id. */
    quint32 id;

    /** The scene name. */
    QString name;

    /** The user id of the scene creator. */
    quint32 creatorId;

    /** The name of the scene creator. */
    QString creatorName;

    /** The time at which the scene was created. */
    QDateTime created;

    /** The width of the scroll region. */
    quint16 scrollWidth;

    /** The height of the scroll region. */
    quint16 scrollHeight;

    /** The scene blocks. */
    QHash<QPoint, Block> blocks;

    /** Keys of blocks added, updated, and removed since the last flush. */
    QSet<QPoint> added, updated, removed;

    /**
     * Sets the value at the specified location.
     */
    void set (const QPoint& pos, int character);

    /**
     * Returns the value at the specified location.
     */
    int get (const QPoint& pos) const;

    /**
     * Checks whether the blocks have changed.
     */
    bool dirty () const { return !(added.isEmpty() && updated.isEmpty() && removed.isEmpty()); }

    /**
     * Clears the update sets.
     */
    void clean ();
};

Q_DECLARE_METATYPE(SceneRecord)

/** A record for the lack of a scene. */
const SceneRecord NoScene = { 0 };

/**
 * Describes a scene for search purposes.
 */
class SceneDescriptor
{
public:

    /** The scene id. */
    quint32 id;

    /** The scene name. */
    QString name;

    /** The user id of the scene creator. */
    quint32 creatorId;

    /** The name of the scene creator. */
    QString creatorName;

    /** The time at which the scene was created. */
    QDateTime created;
};

/** A list of scene descriptors that we'll register with the metatype system. */
typedef QList<SceneDescriptor> SceneDescriptorList;

#endif // SCENE_REPOSITORY
