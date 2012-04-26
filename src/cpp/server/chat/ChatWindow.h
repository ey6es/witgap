//
// $Id$

#ifndef CHAT_WINDOW
#define CHAT_WINDOW

#include <QHash>
#include <QMap>
#include <QPair>
#include <QStringList>

#include "ui/Window.h"

class ChatCommand;
class Label;
class TextField;
class TranslationKey;

/**
 * The window in which chat messages are displayed.
 */
class ChatWindow : public Window
{
    Q_OBJECT

public:

    /** The available modes of speech. */
    enum SpeakMode { NormalMode, EmoteMode, ShoutMode, BroadcastMode, TellMode };

    /**
     * Initializes the window.
     */
    ChatWindow (Session* parent);

    /**
     * Displays a message.
     */
    Q_INVOKABLE void display (
        const QString& speaker, const QString& message, ChatWindow::SpeakMode mode);

    /**
     * Translates and displays message text.
     */
    Q_INVOKABLE void display (const TranslationKey& key);

    /**
     * Displays message text.
     */
    void display (const QString& text);

    /**
     * Clears the display.
     */
    void clear ();
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
     * Returns the command name and handler for the specified command (or command prefix), or
     * a default-constructed pair if not found/ambiguous (in which case a message will be posted
     * to the display).
     */
    QPair<QString, ChatCommand*> getCommand (const QString& cmd) const;

    /**
     * Returns a reference to the active command map.
     */
    const QMap<QString, ChatCommand*>& commands () const { return _commands; }

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
    QMap<QString, ChatCommand*> _commands;
};

#endif // CHAT_WINDOW
