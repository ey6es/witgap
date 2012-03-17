//
// $Id$

#ifndef SCENE_REPOSITORY
#define SCENE_REPOSITORY

#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QPoint>

#include "util/General.h"

class Callback;

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
     * Inserts a new scene into the database.  The callback will receive the {@link SceneRecord}.
     */
    Q_INVOKABLE void insertScene (
        const QString& name, quint32 creatorId, const Callback& callback);

    /**
     * Loads a scene.  The callback will receive the {@link SceneRecord}.
     */
    Q_INVOKABLE void loadScene (quint32 id, const Callback& callback);
};

/**
 * Holds the metadata associated with a scene.
 */
class SceneRecord
{
public:

    /** The scene id. */
    quint32 id;

    /** The scene name. */
    QString name;

    /** The user id of the scene creator. */
    quint32 creatorId;

    /** The time at which the scene was created. */
    QDateTime created;

    /** The width of the scroll region. */
    quint16 scrollWidth;

    /** The height of the scroll region. */
    quint16 scrollHeight;

    /** The scene data. */
    QHash<QPoint, QIntVector> data;
};

Q_DECLARE_METATYPE(SceneRecord)

/** A record for the lack of a scene. */
const SceneRecord NoScene = { 0 };

#endif // SCENE_REPOSITORY
