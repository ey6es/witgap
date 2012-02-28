//
// $Id$

#ifndef SESSION_REPOSITORY
#define SESSION_REPOSITORY

#include <QObject>

#include "util/Callback.h"

/**
 * Handles database queries associated with sessions.
 */
class SessionRepository : public QObject
{
    Q_OBJECT

public:

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
