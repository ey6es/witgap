//
// $Id$

#include <QTranslator>

#include "admin/GenerateInvitesDialog.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("GenerateInvitesDialog", __VA_ARGS__)

/** Expression for unsigned ints. */
const QRegExp UIntExp("\\d{0,10}");

GenerateInvitesDialog::GenerateInvitesDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));
    
    TableLayout* layout = new TableLayout(2);
    layout->stretchColumns().insert(1);
    Container* cont = new Container(layout);
    addChild(cont);
    cont->addChild(new Label(tr("Description:")));
    cont->addChild(_description = new TextField());
    cont->addChild(new Label(tr("Count:")));
    cont->addChild(_count = new TextField(20, new RegExpDocument(UIntExp, "1", 10), true));
    cont->addChild(new Label(tr("Admin:")));
    cont->addChild(_admin = new CheckBox());
    cont->addChild(new Label(tr("Insider:")));
    cont->addChild(_insider = new CheckBox());
    
    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    _ok = new Button(tr("OK"));
    connect(_ok, SIGNAL(pressed()), SLOT(generate()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _ok));
    
    setPreferredSize(QSize(45, -1));

    pack();
    center();
}

void GenerateInvitesDialog::generate ()
{
    deleteLater();
}
