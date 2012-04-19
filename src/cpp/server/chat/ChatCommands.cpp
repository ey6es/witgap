//
// $Id$

#include <QHash>
#include <QTranslator>

#include "ServerApp.h"
#include "actor/Pawn.h"
#include "chat/ChatCommands.h"
#include "chat/ChatWindow.h"
#include "net/Session.h"
#include "scene/Scene.h"

// translate through the translator
#define tr(...) translator->translate("ChatCommands", __VA_ARGS__)

// clear this, lest its internal tr() call translate to the above; lupdate will still pick it up
#undef Q_DECLARE_TR_FUNCTIONS
#define Q_DECLARE_TR_FUNCTIONS(x)

void ChatCommand::handle (Session* session, const QString& cmd, const QString& args)
{
    QString result = handle(session, session->translator(), cmd, args);
    if (!result.isEmpty()) {
        session->chatWindow()->display(result);
    }
}

bool AdminChatCommand::canAccess (Session* session)
{
    return session->admin();
}

/**
 * Handles the help command.
 */
class HelpCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("help ?"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 [command]\n"
            "  Lists commands or displays usage.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            QStringList commands = session->chatEntryWindow()->commands().keys();
            return tr("Available commands: %1").arg(commands.join(" "));
        }
        QString hcmd = args.toLower();
        QPair<QString, ChatCommand*> pair = session->chatEntryWindow()->getCommand(hcmd);
        return (pair.second == 0) ? "" : pair.second->usage(translator, pair.first);
    }
};

/**
 * Handles the clear command.
 */
class ClearCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("clear"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1\n"
            "  Clears the chat display.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatWindow()->clear();
        return "";
    }
};

/**
 * Handles the bug command.
 */
class BugCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("bug"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 description\n"
            "  Submits a bug report.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->submitBugReport(args);
        return tr("Report submitted.  Thanks!");
    }
};

/**
 * Handles the say command.
 */
class SayCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("say"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 message\n"
            "  Speaks a message aloud.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->say(args);
        return "";
    }
};

/**
 * Handles the emote command.
 */
class EmoteCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("emote me"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 message\n"
            "  Displays an emote.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->say(args, ChatWindow::EmoteMode);
        return "";
    }
};

/**
 * Handles the shout command.
 */
class ShoutCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("shout"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 message\n"
            "  Shouts a message.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->say(args, ChatWindow::ShoutMode);
        return "";
    }
};

/**
 * Handles the tell command.
 */
class TellCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("tell"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 username message\n"
            "  Sends a message to a single user.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }

        return "";
    }
};

/**
 * Handles the broadcast command.
 */
class BroadcastCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("broadcast"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 message\n"
            "  Broadcasts a message to everyone.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->say(args, ChatWindow::BroadcastMode);
        return "";
    }
};

/**
 * Creates the map from language codes to command maps.
 */
static QHash<QString, CommandMap> createCommandMapMap (ServerApp* app)
{
    ChatCommand* handlers[] = {
        new HelpCommand(), new ClearCommand(), new BugCommand(), new SayCommand(),
        new EmoteCommand(), new ShoutCommand(), new TellCommand(), new BroadcastCommand() };

    QHash<QString, CommandMap> map;
    for (int ii = 0; ii < sizeof(handlers) / sizeof(handlers[0]); ii++) {
        ChatCommand* handler = handlers[ii];
        for (QHash<QString, QTranslator*>::const_iterator it = app->translators().constBegin(),
                end = app->translators().constEnd(); it != end; it++) {
            CommandMap& commands = map[it.key()];
            foreach (const QString& alias, handlers[ii]->aliases(it.value()).split(' ')) {
                commands.insert(alias, handlers[ii]);
            }
        }
    }
    return map;
}

CommandMap getChatCommands (ServerApp* app, const QString& language)
{
    static QHash<QString, CommandMap> maps = createCommandMapMap(app);
    return maps.value(language);
}
