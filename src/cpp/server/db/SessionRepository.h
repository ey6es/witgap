//
// $Id$

#ifndef SESSION_REPOSITORY
#define SESSION_REPOSITORY

#include <QObject>

#include "util/Callback.h"

/**
 * Performs database queries in a separate thread.
 */
class SessionRepository : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates the repository.
     */
    SessionRepository ();

    /**
     * Initializes the repository, performing any necessary migrations.
     */
    void init ();

    /**
     * Validates the specified session token.
     */
    Q_INVOKABLE void validateToken (quint64 id, const QByteArray& token, const Callback& callback);
};

#endif // SESSION_REPOSITORY
