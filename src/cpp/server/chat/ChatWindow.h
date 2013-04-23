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
     * Returns a pointer to the window's IO device, which may be used to write to the window.
     */
    QIODevice* device () const { return _device; }

    /**
     * Returns a reference to the name of the last speaker from whom a tell was received.
     */
    const QString& lastSender () const { return _lastSender; }

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

protected:

    /** The window's IO device. */
    QIODevice* _device;

    /** The name of the last person from whom a tell was received. */
    QString _lastSender;
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
    ChatEntryWindow (Session* parent, const QStringList& history);

    /**
     * Returns a pointer to the window's IO device, which may be used to read from the window.
     */
    QIODevice* device () const { return _device; }

    /**
     * Returns the command name and handler for the specified command (or command prefix), or
     * a default-constructed pair if not found/ambiguous (in which case a message will be posted
     * to the display).
     */
    QPair<QString, ChatCommand*> getCommand (const QString& cmd);

    /**
     * Returns a reference to the active command map.
     */
    const QMap<QString, ChatCommand*>& commands () const { return _commands; }

    /**
     * Adds a command to the history (or moves it to the front, if already present).
     */
    void addToHistory (const QString& cmd);

    /**
     * Returns a reference to the command history.
     */
    const QStringList& history () const { return _history; }

    /**
     * Sets the entry mode.
     */
    void setMode (const QString& label, const QString& prefix, bool matchParentheses = false);

    /**
     * Restores the default entry mode.
     */
    void clearMode ();

    /**
     * Provides a line of input to the running script.
     */
    void provideInput (const QString& line);

    /**
     * Renders the window visible or invisible.
     */
    virtual void setVisible (bool visible);

protected slots:

    /**
     * Called when the field text changes.
     */
    void maybeExitHistory ();

    /**
     * Submits the entered chat if non-empty.
     */
    void maybeSubmit ();

protected:

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /** The window's IO device. */
    QIODevice* _device;

    /** The entry label. */
    Label* _label;

    /** The entry field. */
    TextField* _field;

    /** Maps command aliases to handlers. */
    QMap<QString, ChatCommand*> _commands;

    /** The command history. */
    QStringList _history;

    /** The current index in the command history, or -1 if not in the history. */
    int _historyIdx;

    /** The command stored when browsing the history. */
    QString _stored;

    /** The mode prefix. */
    QString _prefix;
};

#endif // CHAT_WINDOW
