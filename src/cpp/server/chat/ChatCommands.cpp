//
// $Id$

#include <QHash>
#include <QMetaObject>
#include <QTranslator>

#include "ServerApp.h"
#include "actor/Pawn.h"
#include "chat/ChatCommands.h"
#include "chat/ChatWindow.h"
#include "http/ImportExportManager.h"
#include "net/Session.h"
#include "peer/PeerManager.h"
#include "scene/Scene.h"
#include "script/Evaluator.h"

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
 * Handles the /help command.
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
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            QString list;
            const QMap<QString, ChatCommand*>& commands = session->chatEntryWindow()->commands();
            for (QMap<QString, ChatCommand*>::const_iterator it = commands.constBegin(),
                    end = commands.constEnd(); it != end; it++) {
                if (it.value()->canAccess(session)) {
                    if (!list.isEmpty()) {
                        list += ' ';
                    }
                    list += it.key();
                }
            }
            return tr("Available commands: %1").arg(list);
        }
        QString hcmd = args.toLower();
        QPair<QString, ChatCommand*> pair = session->chatEntryWindow()->getCommand(hcmd);
        return (pair.second == 0) ? "" : pair.second->usage(translator, pair.first);
    }
};

/**
 * Handles the /clear command.
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
        session->chatEntryWindow()->addToHistory("/" + cmd);
        session->chatWindow()->clear();
        return "";
    }
};

/**
 * Handles the /bug command.
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
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->submitBugReport(args);
        return tr("Report submitted.  Thanks!");
    }
};

/**
 * Handles the /say command.
 */
class SayCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("say"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 [message]\n"
            "  Speaks a message aloud/switches to say mode.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            session->chatEntryWindow()->clearMode();
        } else {
            session->say(args);
        }
        return "";
    }
};

/**
 * Handles the /emote command.
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
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->say(args, ChatWindow::EmoteMode);
        return "";
    }
};

/**
 * Handles the /shout command.
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
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->say(args, ChatWindow::ShoutMode);
        return "";
    }
};

/**
 * Handles the /tell command.
 */
class TellCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("tell"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 username [message]\n"
            "  Sends a message to a single user/switches to tell mode.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        if (args.isEmpty()) {
            session->chatEntryWindow()->addToHistory("/" + cmd + " ");
            return usage(translator, cmd);
        }
        int idx = args.indexOf(' ');
        if (idx == -1) {
            QString prefix = "/" + cmd + " " + args;
            session->chatEntryWindow()->addToHistory(prefix + " ");
            session->chatEntryWindow()->setMode(tr("Tell %1:").arg(args), prefix);
            return "";
        }
        QString recipient = args.left(idx);
        if (recipient.toLower() == session->user().name.toLower()) {
            session->chatEntryWindow()->addToHistory("/" + cmd + " ");
            return tr("Talking to yourself is a sign of insanity.");
        }
        session->chatEntryWindow()->addToHistory("/" + cmd + " " + recipient + " ");
        session->tell(recipient, args.mid(idx + 1));
        return "";
    }
};

/**
 * Handles the /respond command.
 */
class RespondCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("respond"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 [message]\n"
            "  Responds to the last tell/switches to respond mode.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            session->chatEntryWindow()->setMode(tr("Respond:"), "/" + cmd);
            return "";
        }
        QString sender = session->chatWindow()->lastSender();
        if (sender.isEmpty()) {
            return tr("You have not received a tell.");
        }
        session->tell(sender, args);
        return "";
    }
};

/**
 * Handles the /mute command.
 */
class MuteCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("mute"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 username\n"
            "  Hides all messages from the user.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->mute(args);
        return "";
    }
};

/**
 * Handles the /unmute command.
 */
class UnmuteCommand : public ChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("unmute"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 username\n"
            "  Removes the user from your mute list.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->unmute(args);
        return "";
    }
};

/**
 * Handles the /broadcast command.
 */
class BroadcastCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("broadcast"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 [message]\n"
            "  Broadcasts a message to everyone/switches to broadcast mode.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            session->chatEntryWindow()->setMode(tr("Broadcast:"), "/" + cmd);
        } else {
            session->say(args, ChatWindow::BroadcastMode);
        }
        return "";
    }
};

/**
 * Handles the /reboot command.
 */
class RebootCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("reboot"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 minutes [message]\n"
            "  Schedules or cancels a system reboot.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            session->app()->peerManager()->invoke(session->app(), "cancelReboot()");
            return tr("Reboot canceled.");
        }
        int idx = args.indexOf(' ');
        QString number, message;
        if (idx == -1) {
            number = args;
        } else {
            number = args.left(idx);
            message = args.mid(idx + 1);
        }
        bool ok;
        int minutes = number.toInt(&ok);
        if (!ok) {
            return usage(translator, cmd);
        }
        session->app()->peerManager()->invoke(session->app(), "scheduleReboot(quint32,QString)",
            Q_ARG(quint32, minutes), Q_ARG(const QString&, message));
        return "Reboot scheduled.";
    }
};

/**
 * Handles the /sgo command.
 */
class SGoCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("sgo"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 scene name\n"
            "  Moves to the named scene.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->moveToScene(args);
        return "";
    }
};

/**
 * Handles the /zgo command.
 */
class ZGoCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("zgo"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 zone name\n"
            "  Moves to the named zone.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->moveToZone(args);
        return "";
    }
};

/**
 * Handles the /pgo command.
 */
class PGoCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("pgo"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 player name\n"
            "  Moves to the named player.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        if (args.toLower() != session->user().name.toLower()) {
            session->moveToPlayer(args);
        }
        return "";
    }
};

/**
 * Handles the /summon command.
 */
class SummonCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("summon"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 player name\n"
            "  Moves the named player to you.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        if (args.toLower() != session->user().name.toLower()) {
            session->summonPlayer(args);
        }
        return "";
    }
};

/**
 * Handles the /spawn command.
 */
class SpawnCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("spawn"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 actor name\n"
            "  Spawns an actor.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }
        session->spawnActor(args);
        return "";
    }
};

/**
 * Handles the /eval command.
 */
class EvalCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("eval"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 [expression]\n"
            "  Evaluates the expression/switches to eval mode.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            session->chatEntryWindow()->setMode(tr("Eval:"), "/" + cmd, true);

        } else {
            session->evaluator()->evaluate(args);
        }
        return "";
    }
};

/**
 * Handles the /upload command.
 */
class UploadCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("upload"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1\n"
            "  Accepts a script upload to execute.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd);
        QMetaObject::invokeMethod(session->app()->importExportManager(), "addImport",
            Q_ARG(const QString&, "script.scm"),
            Q_ARG(const Callback&, Callback(session, "runScript(QByteArray)")),
            Q_ARG(const Callback&, Callback(session, "openUrl(QUrl)")));
        return "";
    }
};

/**
 * Handles the /input command.
 */
class InputCommand : public AdminChatCommand
{
    Q_DECLARE_TR_FUNCTIONS(ChatCommands)

public:

    virtual QString aliases (QTranslator* translator) { return tr("input"); }

    virtual QString usage (QTranslator* translator, const QString& cmd) {
        return tr("Usage: /%1 line\n"
            "  Provides input to script/switches to input mode.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        session->chatEntryWindow()->addToHistory("/" + cmd + " ");
        if (args.isEmpty()) {
            session->chatEntryWindow()->setMode(tr("Input:"), "/" + cmd, false);

        } else {
            session->chatEntryWindow()->provideInput(args);
        }
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
        new EmoteCommand(), new ShoutCommand(), new TellCommand(), new RespondCommand(),
        new MuteCommand(), new UnmuteCommand(), new BroadcastCommand(), new RebootCommand(),
        new SGoCommand(), new ZGoCommand(), new PGoCommand(), new SummonCommand(),
        new SpawnCommand(), new EvalCommand(), new UploadCommand(), new InputCommand() };

    QHash<QString, CommandMap> map;
    for (int ii = 0; ii < sizeof(handlers) / sizeof(ChatCommand*); ii++) {
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
