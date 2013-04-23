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

/**
 * A device that allows writing to the chat window.
 */
class ChatWindowDevice : public QIODevice
{
public:

    /**
     * Creates a new chat window device.
     */
    ChatWindowDevice (ChatWindow* window);

protected:

    /**
     * Attempts to read data from the device.
     */
    virtual qint64 readData (char* data, qint64 maxSize);

    /**
     * Attempts to write data to the device.
     */
    virtual qint64 writeData (const char* data, qint64 maxSize);

    /** Buffered data. */
    QByteArray _buffer;
};

ChatWindowDevice::ChatWindowDevice (ChatWindow* window) :
    QIODevice(window)
{
    setOpenMode(WriteOnly);
}

qint64 ChatWindowDevice::readData (char* data, qint64 maxSize)
{
    return -1;
}

qint64 ChatWindowDevice::writeData (const char* data, qint64 maxSize)
{
    _buffer.append(data, maxSize);
    int idx = _buffer.lastIndexOf('\n');
    if (idx != -1) {
        _buffer[idx] = 0x0;
        static_cast<ChatWindow*>(parent())->display(_buffer.constData());
        _buffer.remove(0, idx + 1);
    }
    return maxSize;
}

ChatWindow::ChatWindow (Session* parent) :
    Window(parent, 1),
    _device(new ChatWindowDevice(this))
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
            _lastSender = speaker;
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

/**
 * A device that allows reading from the chat entry window.
 */
class ChatEntryWindowDevice : public QIODevice
{
public:

    /**
     * Creates a new chat entry window device.
     */
    ChatEntryWindowDevice (ChatEntryWindow* window);

    /**
     * Returns a pointer to the session.
     */
    Session* session () const { return static_cast<ChatEntryWindow*>(parent())->session(); }

    /**
     * Adds a line of input to the buffer.
     */
    void appendToBuffer (const QByteArray& data) { _buffer.append(data); emit readyRead(); }

    /**
     * Checks whether we can read a line of input.
     */
    virtual bool canReadLine () const;

protected:

    /**
     * Attempts to read data from the device.
     */
    virtual qint64 readData (char* data, qint64 maxSize);

    /**
     * Attepts to read an entire line of data.
     */
    virtual qint64 readLineData (char* data, qint64 maxSize);

    /**
     * Attempts to write data to the device.
     */
    virtual qint64 writeData (const char* data, qint64 maxSize);

    /** Buffered data remaining to be read. */
    QByteArray _buffer;
};

ChatEntryWindowDevice::ChatEntryWindowDevice (ChatEntryWindow* window) :
    QIODevice(window)
{
    setOpenMode(ReadOnly);
}

bool ChatEntryWindowDevice::canReadLine () const
{
    if (!_buffer.isEmpty() || QIODevice::canReadLine()) {
        return true;
    }
    // it's a little hacky, but expressing interest in reading a line basically means
    // we want to switch to input mode
    ChatEntryWindow* window = static_cast<ChatEntryWindow*>(parent());
    window->setVisible(true);
    window->setMode(tr("Input:"), "/input", false);
    return false;
}

qint64 ChatEntryWindowDevice::readData (char* data, qint64 maxSize)
{
    int size = qMin((int)maxSize, _buffer.size());
    qCopy(_buffer.constData(), _buffer.constData() + size, data);
    _buffer.remove(0, size);
    return size;
}

qint64 ChatEntryWindowDevice::readLineData (char* data, qint64 maxSize)
{
    qint64 idx = _buffer.indexOf('\n');
    return (idx == -1) ? 0 : readData(data, qMin(maxSize, idx + 1));
}

qint64 ChatEntryWindowDevice::writeData (const char* data, qint64 maxSize)
{
    return -1;
}

ChatEntryWindow::ChatEntryWindow (Session* parent, const QStringList& history) :
    Window(parent, 1),
    _device(new ChatEntryWindowDevice(this)),
    _history(history),
    _historyIdx(-1)
{
    setLayout(new BoxLayout(Qt::Horizontal, BoxLayout::HStretch));

    addChild(_label = new Label(tr("Say:")), BoxLayout::Fixed);
    addChild(_field = new TextField());
    connect(_field, SIGNAL(textChanged()), SLOT(maybeExitHistory()));
    connect(_field, SIGNAL(enterPressed()), SLOT(maybeSubmit()));

    setVisible(false);

    _commands = getChatCommands(parent->app(), parent->locale());
}

QPair<QString, ChatCommand*> ChatEntryWindow::getCommand (const QString& cmd)
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

/** The maximum size of the command history. */
const int maxHistorySize = 10;

void ChatEntryWindow::addToHistory (const QString& cmd)
{
    int idx = _history.indexOf(cmd);
    if (idx != -1) {
        _history.move(idx, _history.size() - 1);
    } else {
        _history.append(cmd);
        if (_history.size() > maxHistorySize) {
            _history.removeFirst();
        }
    }
}

void ChatEntryWindow::setMode (const QString& label, const QString& prefix, bool matchParentheses)
{
    _label->setText(label);
    _prefix = prefix;
    _field->setMatchParentheses(matchParentheses);
}

void ChatEntryWindow::clearMode ()
{
    setMode(tr("Say:"), "");
}

void ChatEntryWindow::provideInput (const QString& line)
{
    static_cast<ChatEntryWindowDevice*>(_device)->appendToBuffer(line.toAscii() += '\n');
}

void ChatEntryWindow::setVisible (bool visible)
{
    if (_visible != visible) {
        Window::setVisible(visible);

        if (visible) {
            // request focus for the text field when shown
            _field->requestFocus();

        } else {
            // exit the history and revert mode when hidden
            if (_historyIdx != -1) {
                _field->setText(_stored);
                _historyIdx = -1;
            }
            clearMode();
        }
    }
}

void ChatEntryWindow::maybeExitHistory ()
{
    _historyIdx = -1;
}

void ChatEntryWindow::maybeSubmit ()
{
    QString text = _field->text().simplified();
    _field->setText("");
    if (text.isEmpty()) {
        setVisible(false);
        return;
    }
    _historyIdx = -1;

    Session* session = this->session();
    if (text.at(0) != '/') {
        if (_prefix.isEmpty()) {
            session->say(text);
            return;
        }
        text = _prefix + " " + text;
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
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers != Qt::ShiftModifier && modifiers != Qt::NoModifier) {
        Window::keyPressEvent(e);
        return;
    }
    switch (e->key()) {
        case Qt::Key_Up:
            if (_historyIdx == -1) {
                if (!_history.isEmpty()) {
                    _stored = _field->text();
                    _field->setText(_history.at(_historyIdx = _history.size() - 1));
                }
            } else if (_historyIdx > 0) {
                _field->setText(_history.at(--_historyIdx));
            }
            break;

        case Qt::Key_Down:
            if (_historyIdx != -1) {
                if (_historyIdx == _history.size() - 1) {
                    _field->setText(_stored);
                    _historyIdx = -1;

                } else {
                    _field->setText(_history.at(++_historyIdx));
                }
            }
            break;

        case Qt::Key_Escape:
            setVisible(false);
            break;

        default:
            Window::keyPressEvent(e);
            break;
    }
}
