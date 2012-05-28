//
// $Id$

#include <QTranslator>

#include "RuntimeConfig.h"
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

    addChild(_editor = new ObjectEditor(new RuntimeConfig(this),
        RuntimeConfig::staticMetaObject.propertyOffset()));

    Button* ok = new Button(tr("OK"));
    connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, ok));

    pack();
    center();
}
