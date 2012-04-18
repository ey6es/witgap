//
// $Id$

#include <QKeyEvent>
#include <QTranslator>
#include <QVarLengthArray>
#include <QtDebug>

#include "ChatWindow.h"
#include "ServerApp.h"
#include "actor/Pawn.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"

// translate through the session
#define tr(...) this->session()->translate("ChatWindow", __VA_ARGS__)

ChatWindow::ChatWindow (Session* parent) :
    Window(parent, 1)
{
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignBottom, 0));
    setBackground(0);
}

void ChatWindow::display (const QString& speaker, const QString& message)
{
    display(tr("%1 says, \"%2\"").arg(speaker, message));
}

void ChatWindow::display (const QString& text)
{
    // break the text up into lines and append to the list
    int length = 0;
    QRect innerRect = this->innerRect();
    int wlimit = innerRect.width();
    const QChar* start = text.constData(), *whitespace = 0;
    bool wrun = false;
    for (const QChar* ptr = start, *end = ptr + text.size(); ptr < end; ptr++) {
        QChar value = *ptr;
        if (value != '\n') {
            if (value == ' ') {
                if (!wrun) { // it's the start of a run of whitespace
                    wrun = true;
                    whitespace = ptr;
                }
            } else {
                wrun = false;
            }
            if (++length <= wlimit) {
                continue;
            }
            if (whitespace == 0) { // nowhere to break
                length--;

            } else {
                length -= (ptr - whitespace + 1);

                // consume any whitespace after the line
                for (ptr = whitespace + 1; ptr < end && *ptr == ' '; ptr++);
            }
            ptr--;
        }
        addChild(new Label(QString(start, length)));
        start = ptr + 1;
        length = 0;
        whitespace = 0;
        wrun = false;
    }
    addChild(new Label(QString(start, length)));

    // remove any lines beyond the limit
    while (_children.size() > innerRect.height()) {
        removeChildAt(0);
    }
}

/**
 * Base class for chat command handlers.
 */
class ChatCommandHandler
{
public:

    /**
     * Checks whether the specified session can access this command.
     */
    virtual bool canAccess (Session* session) { return true; }

    /**
     * Handles a command.
     */
    virtual void handle (Session* session, const QString& cmd, const QString& args) = 0;
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
    virtual bool canAccess (Session* session) { return session->admin(); }
};

/** Type for map from command alias to handler pointer. */
typedef QMap<QString, ChatCommandHandler*> CommandHandlerMap;

static QHash<QString, CommandHandlerMap> createCommandHandlerMapMap (ServerApp* app);

/**
 * Returns the command handler map for the specified language.
 */
static CommandHandlerMap getCommandHandlers (ServerApp* app, const QString& language)
{
    static QHash<QString, CommandHandlerMap> maps = createCommandHandlerMapMap(app);
    return maps.value(language);
}

ChatEntryWindow::ChatEntryWindow (Session* parent) :
    Window(parent, 1)
{
    setLayout(new BoxLayout(Qt::Horizontal, BoxLayout::HStretch));

    addChild(_label = new Label(tr("Say:")), BoxLayout::Fixed);
    addChild(_field = new TextField());
    connect(_field, SIGNAL(enterPressed()), SLOT(maybeSubmit()));

    setVisible(false);

    _commands = getCommandHandlers(parent->app(), parent->locale());
}

void ChatEntryWindow::setVisible (bool visible)
{
    if (_visible != visible) {
        Window::setVisible(visible);

        // request focus for the text field when made visible
        if (visible) {
            _field->requestFocus();
        }
    }
}

void ChatEntryWindow::maybeSubmit ()
{
    QString text = _field->text().simplified();
    _field->setText("");
    if (text.isEmpty()) {
        setVisible(false);
        return;
    }
    Session* session = this->session();
    if (text.at(0) != '/') {
        Scene* scene = session->scene();
        if (scene != 0) {
            Pawn* pawn = session->pawn();
            if (pawn != 0) {
                scene->say(pawn->position(), session->user().name, text);
            }
        }
        return;
    }
    // process as command or command prefix
    QString cmd, args;
    int idx = text.indexOf(' ');
    if (idx == -1) {
        cmd = text.mid(1).toLower();
        args = "";
    } else {
        cmd = text.mid(1, idx - 1).toLower();
        args = text.mid(idx + 1);
    }
    if (cmd.isEmpty()) {
        return;
    }
    QVarLengthArray<CommandHandlerMap::const_iterator, 4> matches;
    for (CommandHandlerMap::const_iterator it = _commands.lowerBound(cmd),
            end = _commands.constEnd(); it != end; it++) {
        if (it.key() == cmd) { // exact match
            if (it.value()->canAccess(session)) {
                it.value()->handle(session, cmd, args);
                return;
            }
        } else if (it.key().startsWith(cmd)) { // partial match
            if (it.value()->canAccess(session)) {
                matches.append(it);
            }
        } else { // no match
            break;
        }
    }
    int nmatches = matches.size();
    if (nmatches == 0) { // no partial matches
        session->chatWindow()->display(tr("Unknown command '%1'.  Try '/help'.").arg(cmd));

    } else if (nmatches == 1) { // only one partial match
        matches[0].value()->handle(session, cmd, args);

    } else { // multiple partial matches
        QString list = matches[0].key();
        for (int ii = 1; ii < nmatches; ii++) {
            list.append(' ').append(matches[ii].key());
        }
        session->chatWindow()->display(tr("Ambiguous command.  Did you mean '%1'?").arg(list));
    }
}

void ChatEntryWindow::keyPressEvent (QKeyEvent* e)
{
    // hide on escape
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (e->key() == Qt::Key_Escape &&
            (modifiers == Qt::ShiftModifier || modifiers == Qt::NoModifier)) {
        setVisible(false);
    } else {
        Window::keyPressEvent(e);
    }
}

// here, we want to translate through the translator we've been passed
#undef tr
#define tr(...) session->translate("ChatCommands", __VA_ARGS__)

/**
 * Handles help command.
 */
class HelpHandler : public ChatCommandHandler
{
public:

    /**
     * Handles a command.
     */
    virtual void handle (Session* session, const QString& cmd, const QString& args);
};

void HelpHandler::handle (Session* session, const QString& cmd, const QString& args)
{
    session->chatWindow()->display(tr("Hi there!"));
}

static void addHandler (ServerApp* app, QHash<QString, CommandHandlerMap>& map,
    const char* aliases, ChatCommandHandler* handler)
{
    for (QHash<QString, QTranslator*>::const_iterator it = app->translators().constBegin(),
            end = app->translators().constEnd(); it != end; it++) {
        CommandHandlerMap& commands = map[it.key()];
        foreach (const QString& alias, it.value()->translate("ChatCommands", aliases).split(' ')) {
            commands.insert(alias, handler);
        }
    }
}

// here, we want the untranslated string
#undef tr
#define tr(x) x

/**
 * Creates the map from language codes to command handler maps.
 */
static QHash<QString, CommandHandlerMap> createCommandHandlerMapMap (ServerApp* app)
{
    QHash<QString, CommandHandlerMap> map;
    addHandler(app, map, tr("help ?"), new HelpHandler());
//    add(tr("emote me"), new EmoteHandler());
    return map;
}
