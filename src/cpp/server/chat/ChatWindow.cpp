//
// $Id$

#include <QKeyEvent>
#include <QMetaType>
#include <QTranslator>
#include <QVarLengthArray>
#include <QtDebug>

#include "actor/Pawn.h"
#include "chat/ChatCommands.h"
#include "chat/ChatWindow.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"
#include "util/General.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("ChatWindow", __VA_ARGS__)

// register our types with the metatype system
int speakModeType = qRegisterMetaType<ChatWindow::SpeakMode>("ChatWindow::SpeakMode");

ChatWindow::ChatWindow (Session* parent) :
    Window(parent, 1)
{
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignBottom, 0));
    setBackground(0);
}

void ChatWindow::display (const QString& speaker, const QString& message, SpeakMode mode)
{
    QString format;
    switch (mode) {
        case NormalMode:
            format = tr("%1 says, \"%2\"");
            break;

        case EmoteMode:
            format = tr("%1 %2");
            break;

        case ShoutMode:
            format = tr("%1 shouts, \"%2\"");
            break;

        case BroadcastMode:
            format = tr("%1 broadcasts, \"%2\"");
            break;

        case TellMode:
            format = tr("%1 tells you, \"%2\"");
            break;
    }
    display(format.arg(speaker, message));
}

void ChatWindow::display (const TranslationKey& key)
{
    display(key.translate(session()->translator()));
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

void ChatWindow::clear ()
{
    removeAllChildren();
}

ChatEntryWindow::ChatEntryWindow (Session* parent) :
    Window(parent, 1)
{
    setLayout(new BoxLayout(Qt::Horizontal, BoxLayout::HStretch));

    addChild(_label = new Label(tr("Say:")), BoxLayout::Fixed);
    addChild(_field = new TextField());
    connect(_field, SIGNAL(enterPressed()), SLOT(maybeSubmit()));

    setVisible(false);

    _commands = getChatCommands(parent->app(), parent->locale());
}

QPair<QString, ChatCommand*> ChatEntryWindow::getCommand (const QString& cmd) const
{
    Session* session = this->session();
    QVarLengthArray<CommandMap::const_iterator, 4> matches;
    for (CommandMap::const_iterator it = _commands.lowerBound(cmd),
            end = _commands.constEnd(); it != end; it++) {
        if (it.key() == cmd) { // exact match
            if (it.value()->canAccess(session)) {
                return QPair<QString, ChatCommand*>(cmd, it.value());
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
        return QPair<QString, ChatCommand*>(matches[0].key(), matches[0].value());

    } else { // multiple partial matches
        QString list = matches[0].key();
        ChatCommand* common = matches[0].value();
        for (int ii = 1; ii < nmatches; ii++) {
            list.append(' ').append(matches[ii].key());
            if (matches[ii].value() != common) {
                common = 0;
            }
        }
        if (common != 0) { // false alarm; they all point to the same handler
            return QPair<QString, ChatCommand*>(matches[0].key(), common);
        }
        session->chatWindow()->display(tr("Ambiguous command.  Did you mean '%1'?").arg(list));
    }
    return QPair<QString, ChatCommand*>();
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
        session->say(text);
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
    QPair<QString, ChatCommand*> pair = getCommand(cmd);
    if (pair.second != 0) {
        pair.second->handle(session, pair.first, args);
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
