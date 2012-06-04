//
// $Id$

#include <QTranslator>

#include "ServerApp.h"
#include "admin/RuntimeConfigDialog.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Layout.h"
#include "ui/ObjectEditor.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("RuntimeConfigDialog", __VA_ARGS__)

RuntimeConfigDialog::RuntimeConfigDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    RuntimeConfig* copy = new RuntimeConfig(this);
    _synchronizer = new ObjectSynchronizer(parent->app()->runtimeConfig(true), copy);

    addChild(_editor = new ObjectEditor(copy, RuntimeConfig::staticMetaObject.propertyOffset()));

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    Button* apply = new Button(tr("Apply"));
    connect(apply, SIGNAL(pressed()), SLOT(apply()));
    Button* ok = new Button(tr("OK"));
    connect(ok, SIGNAL(pressed()), SLOT(apply()));
    connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, apply, ok));

    pack();
    center();
}

void RuntimeConfigDialog::apply ()
{
    _editor->apply();
    emit _synchronizer->apply();
}
