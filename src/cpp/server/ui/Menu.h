//
// $Id$

#ifndef MENU
#define MENU

#include "ui/Window.h"

class Button;

/**
 * A pop-up menu.
 */
class Menu : public Window
{
    Q_OBJECT

public:

    /**
     * Creates a new menu.
     */
    Menu (Session* parent);

    /**
     * Adds a button that will create an instance of the specified type with the provided
     * arguments.
     */
    Button* addButton (const QString& label, const QMetaObject* metaObject,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Adds a button and connects its press signal to the specified slot of the given object.
     */
    Button* addButton (const QString& label, QObject* obj, const char* method);

protected:

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Handles a key release event.
     */
    virtual void keyReleaseEvent (QKeyEvent* e);
};

#endif // MENU
