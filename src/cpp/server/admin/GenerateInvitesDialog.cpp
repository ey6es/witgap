//
// $Id$

#include <QTranslator>

#include "ServerApp.h"
#include "admin/GenerateInvitesDialog.h"
#include "db/DatabaseThread.h"
#include "db/UserRepository.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TabbedPane.h"
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
    
    
    addChild(_tabs = new TabbedPane(Qt::Vertical));
    
    Container* link = new Container(new TableLayout(2));
    static_cast<TableLayout*>(link->layout())->stretchColumns().insert(1);
    _tabs->addTab(tr("Link"), link);
    link->addChild(new Label(tr("Description:")));
    link->addChild(_description = new TextField());
    link->addChild(new Label(tr("Count:")));
    link->addChild(_count = new TextField(20, new RegExpDocument(UIntExp, "1", 10), true));
    
    Container* emails = new Container(new TableLayout(2));
    static_cast<TableLayout*>(emails->layout())->stretchColumns().insert(1);
    _tabs->addTab(tr("Emails"), emails);
    
    Container* cont = new Container(new TableLayout(2));
    static_cast<TableLayout*>(cont->layout())->stretchColumns().insert(1);
    addChild(cont);
    
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
    // send the request off to the database
    int flags = (_admin->selected() ? UserRecord::Admin : 0) |
        (_insider->selected() ? UserRecord::Insider : 0);
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "insertInvite",
        Q_ARG(const QString&, _description->text()), Q_ARG(int, flags),
        Q_ARG(int, _count->text().toInt()), Q_ARG(const Callback&,
            Callback(_this, "showInviteUrl(QString)")));
}

void GenerateInvitesDialog::showInviteUrl (const QString& url)
{
    session()->showInfoDialog(tr("Invite URL: %1").arg(url));
    deleteLater();
}
