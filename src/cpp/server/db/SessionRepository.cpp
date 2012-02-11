//
// $Id$

#include <QtDebug>

#include "db/SessionRepository.h"

SessionRepository::SessionRepository ()
{
}

void SessionRepository::init ()
{
}

void SessionRepository::validateToken (
    quint64 id, const QByteArray& token, const Callback& callback)
{
    callback.invoke(Q_ARG(quint64, id), Q_ARG(const QByteArray&, token));
}
