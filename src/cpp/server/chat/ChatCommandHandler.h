//
// $Id$

#ifndef CHAT_COMMAND_HANDLER
#define CHAT_COMMAND_HANDLER

#include <QMap>
#include <QString>

class QTranslator;

class ServerApp;
class Session;

/**
 * Base class for chat command handlers.
 */
class ChatCommandHandler
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
 * Base class for admin command handlers.
 */
class AdminChatCommandHandler : public ChatCommandHandler
{
public:

    /**
     * Checks whether the specified session can access this command.
     */
    virtual bool canAccess (Session* session);
};

/** Type for map from command alias to handler pointer. */
typedef QMap<QString, ChatCommandHandler*> CommandHandlerMap;

/**
 * Returns the command handler map for the specified language.
 */
CommandHandlerMap getChatCommandHandlers (ServerApp* app, const QString& language);

#endif // CHAT_COMMAND_HANDLER
