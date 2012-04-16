//
// $Id$

#include <QKeyEvent>
#include <QtDebug>

#include "ChatWindow.h"
#include "actor/Pawn.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"

// translate through the session
#define tr(...) session()->translate("ChatWindow", __VA_ARGS__)

ChatWindow::ChatWindow (Session* parent) :
    Window(parent, 1)
{
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignBottom, 0));

    setBackground(0);
}

void ChatWindow::displayMessage (const QString& speaker, const QString& message)
{
    addChild(new Label(tr("%1 says, \"%2\"").arg(speaker, message)));
}

ChatEntryWindow::ChatEntryWindow (Session* parent) :
    Window(parent, 1)
{
    setLayout(new BoxLayout(Qt::Horizontal, BoxLayout::HStretch));

    addChild(_label = new Label(tr("Say:")), BoxLayout::Fixed);
    addChild(_field = new TextField());
    connect(_field, SIGNAL(enterPressed()), SLOT(maybeSubmit()));

    setVisible(false);
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
    Scene* scene = session->scene();
    if (scene != 0) {
        Pawn* pawn = session->pawn();
        if (pawn != 0) {
            scene->say(pawn->position(), session->user().name, text);
        }
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
