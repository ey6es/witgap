//
// $Id$

#ifndef CHAT_COMMANDS
#define CHAT_COMMANDS

#include <QMap>
#include <QString>

class QTranslator;

class ServerApp;
class Session;

/**
 * Base class for chat commands.
 */
class ChatCommand
{
public:

    /**
     * Returns the space-delimited list of aliases for the command.
     */
    virtual QString aliases (QTranslator* translator) = 0;

    /**
     * Returns the usage for the command.
     */
    virtual QString usage (QTranslator* translator, const QString& cmd) = 0;

    /**
     * Checks whether the specified session can access this command.
     */
    virtual bool canAccess (Session* session) { return true; }

    /**
     * Handles a command.
     */
    void handle (Session* session, const QString& cmd, const QString& args);

    /**
     * Handles a command.
     *
     * @return a string to display as feedback, or the empty string for none.
     */
    virtual QString handle (
        Session* session, QTranslator* translator, const QString& cmd, const QString& args) = 0;
};

/**
 * Base class for admin commands.
 */
class AdminChatCommand : public ChatCommand
{
public:

    /**
     * Checks whether the specified session can access this command.
     */
    virtual bool canAccess (Session* session);
};

/** Type for map from command alias to handler pointer. */
typedef QMap<QString, ChatCommand*> CommandMap;

/**
 * Returns the command map for the specified language.
 */
CommandMap getChatCommands (ServerApp* app, const QString& language);

#endif // CHAT_COMMANDS
