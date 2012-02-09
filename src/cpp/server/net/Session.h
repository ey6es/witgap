//
// $Id$

#ifndef SESSION
#define SESSION

#include <QObject>

/**
 * Handles a single user session.
 */
class Session : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the session.
     */
    Session ();

    /**
     * Destroys the session.
     */
    ~Session ();
};

#endif // SESSION
