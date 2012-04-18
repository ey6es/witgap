//
// $Id$

#ifndef CHAT_WINDOW
#define CHAT_WINDOW

#include <QHash>
#include <QMap>
#include <QStringList>

#include "ui/Window.h"

class ChatCommandHandler;
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
    void display (const QString& speaker, const QString& message);

    /**
     * Displays message text.
     */
    void display (const QString& text);
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
     * Returns the command handler for the specified command (or command prefix), or
     * 0 if not found/ambiguous (in which case a message will be posted to the display).
     */
    ChatCommandHandler* getCommandHandler (const QString& cmd) const;

    /**
     * Returns a reference to the active command handler map.
     */
    const QMap<QString, ChatCommandHandler*>& commandHandlers () const { return _commandHandlers; }

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

    /** Maps command aliases to handlers. */
    QMap<QString, ChatCommandHandler*> _commandHandlers;
};

#endif // CHAT_WINDOW
