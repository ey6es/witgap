//
// $Id$

#ifndef CHAT_WINDOW
#define CHAT_WINDOW

#include "ui/Window.h"

class Label;
class TextField;

/**
 * The window in which chat messages are displayed.
 */
class ChatWindow : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the window.
     */
    ChatWindow (Session* parent);

    /**
     * Displays a message.
     */
    void displayMessage (const QString& speaker, const QString& message);
};

/**
 * The window in which chat messages are entered.
 */
class ChatEntryWindow : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the window.
     */
    ChatEntryWindow (Session* parent);

    /**
     * Renders the window visible or invisible.
     */
    virtual void setVisible (bool visible);

protected slots:

    /**
     * Submits the entered chat if non-empty.
     */
    void maybeSubmit ();

protected:

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /** The entry label. */
    Label* _label;

    /** The entry field. */
    TextField* _field;
};

#endif // CHAT_WINDOW
