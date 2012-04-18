//
// $Id$

#include <QHash>
#include <QTranslator>

#include "ServerApp.h"
#include "actor/Pawn.h"
#include "chat/ChatCommandHandler.h"
#include "chat/ChatWindow.h"
#include "net/Session.h"
#include "scene/Scene.h"

// translate through the translator
#define tr(...) translator->translate("ChatCommandHandler", __VA_ARGS__)

void ChatCommandHandler::handle (Session* session, const QString& cmd, const QString& args)
{
    QString result = handle(session, session->translator(), cmd, args);
    if (!result.isEmpty()) {
        session->chatWindow()->display(result);
    }
}

bool AdminChatCommandHandler::canAccess (Session* session)
{
    return session->admin();
}

/**
 * Handles help command.
 */
class HelpHandler : public ChatCommandHandler
{
public:

    virtual QString aliases (QTranslator* translator) { return tr("help ?"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 [command]\n"
            "  Lists available commands or displays command usage.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            QStringList commands = session->chatEntryWindow()->commandHandlers().keys();
            return tr("Available commands: %1").arg(commands.join(" "));
        }
        QString hcmd = args.toLower();
        ChatCommandHandler* handler = session->chatEntryWindow()->getCommandHandler(hcmd);
        return (handler == 0) ? "" : handler->usage(translator, hcmd);
    }
};

/**
 * Handles the say command.
 */
class SayHandler : public ChatCommandHandler
{
    virtual QString aliases (QTranslator* translator) { return tr("say speak"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 message\n"
            "  Speaks a message aloud.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        Scene* scene = session->scene();
        if (scene != 0) {
            Pawn* pawn = session->pawn();
            if (pawn != 0) {
                scene->say(pawn->position(), session->user().name, args);
            }
        }
        return "";
    }
};

/**
 * Adds a chat command handler to the provided map.
 */
static void addHandler (
    ServerApp* app, QHash<QString, CommandHandlerMap>& map, ChatCommandHandler* handler)
{
    for (QHash<QString, QTranslator*>::const_iterator it = app->translators().constBegin(),
            end = app->translators().constEnd(); it != end; it++) {
        CommandHandlerMap& commands = map[it.key()];
        foreach (const QString& alias, handler->aliases(it.value()).split(' ')) {
            commands.insert(alias, handler);
        }
    }
}

/**
 * Creates the map from language codes to command handler maps.
 */
static QHash<QString, CommandHandlerMap> createCommandHandlerMapMap (ServerApp* app)
{
    QHash<QString, CommandHandlerMap> map;
    addHandler(app, map, new HelpHandler());
    addHandler(app, map, new SayHandler());
    return map;
}

CommandHandlerMap getChatCommandHandlers (ServerApp* app, const QString& language)
{
    static QHash<QString, CommandHandlerMap> maps = createCommandHandlerMapMap(app);
    return maps.value(language);
}
