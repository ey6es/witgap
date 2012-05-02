//
// $Id$

#include <QHash>
#include <QMetaObject>
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
        return tr("Usage: /%1 username message\n"
            "  Sends a message to a single user.").arg(cmd);
    }

    virtual QString handle (Session* session, QTranslator* translator,
            const QString& cmd, const QString& args) {
        int idx = args.indexOf(' ');
        if (idx == -1) {
            return usage(translator, cmd);
        }
        QString recipient = args.left(idx);
        if (recipient.toLower() == session->user().name.toLower()) {
            return tr("Talking to yourself is a sign of insanity.");
        }
        session->tell(recipient, args.mid(idx + 1));
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
        if (args.isEmpty()) {
            QMetaObject::invokeMethod(session->app(), "cancelReboot");
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
        QMetaObject::invokeMethod(session->app(), "scheduleReboot", Q_ARG(int, minutes),
            Q_ARG(const QString&, message));
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
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }

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
        if (args.isEmpty()) {
            return usage(translator, cmd);
        }

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
        if (args.isEmpty()) {
            return usage(translator, cmd);
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
        if (args.isEmpty()) {
            return usage(translator, cmd);
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
        new EmoteCommand(), new ShoutCommand(), new TellCommand(), new BroadcastCommand(),
        new RebootCommand(), new SGoCommand(), new ZGoCommand(), new PGoCommand(),
        new SummonCommand() };

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
