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
#include "ui/TextComponent.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("GenerateInvitesDialog", __VA_ARGS__)

/** Expression for unsigned ints. */
const QRegExp UIntExp("\\d{0,10}");

GenerateInvitesDialog::GenerateInvitesDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));
    
    addChild(_tabs = new TabbedPane(Qt::Horizontal));
    
    Container* link = new Container(new TableLayout(2, -1, 1, 0, Qt::AlignTop));
    static_cast<TableLayout*>(link->layout())->stretchColumns().insert(1);
    _tabs->addTab(tr("Link"), link);
    link->addChild(new Label(tr("Description:")));
    link->addChild(_description = new TextField());
    link->addChild(new Label(tr("Count:")));
    link->addChild(_count = new TextField(20, new RegExpDocument(UIntExp, "1", 10), true));
    
    _tabs->addTab(tr("Emails"), BoxLayout::createHStretchBox(2, _emails = new TextArea()));
    
    _emails->setText("Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    
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
    if (_tabs->selectedIndex() == 0) {
        QMetaObject::invokeMethod(
            session()->app()->databaseThread()->userRepository(), "insertInvite",
            Q_ARG(const QString&, _description->text()), Q_ARG(int, flags),
            Q_ARG(int, _count->text().toInt()), Q_ARG(const Callback&,
                Callback(_this, "showInviteUrl(QString)")));
    
    } else {
        QStringList emails = _emails->text().split(QRegExp("\\s+"));
        QMetaObject::invokeMethod(
            session()->app()->databaseThread()->userRepository(), "insertInvites",
            Q_ARG(const QStringList&, emails), Q_ARG(int, flags),
            Q_ARG(const Callback&, Callback(_this, "mailInvites(QStringList,QStringList)",
                Q_ARG(const QStringList&, emails))));
    }
}

void GenerateInvitesDialog::showInviteUrl (const QString& url)
{
    Connection* connection = session()->connection();
    if (connection != 0) {
        Connection::evaluateMetaMethod().invoke(connection, Q_ARG(const QString&,
            "window.prompt('Invite URL:', '" + url + "')"));
    }    
    deleteLater();
}

void GenerateInvitesDialog::mailInvites (const QStringList& emails, const QStringList& urls)
{
    _emailsRemaining = emails.size();

    Session* session = this->session();
    for (int ii = 0, nn = emails.size(); ii < nn; ii++) {
        session->app()->sendMail(emails.at(ii), tr("Invitation to Witgap"),
            tr("Hey, come check out the magical world of Witgap!  %1").arg(urls.at(ii)),
            Callback(_this, "emailMaybeSent(QString,QString)",
                Q_ARG(const QString&, emails.at(ii))));
    }    
}

void GenerateInvitesDialog::emailMaybeSent (const QString& email, const QString& error)
{
    if (!error.isEmpty()) {
        session()->showInfoDialog(tr("Error sending to %1: %2.").arg(email, error));
    }
    if (--_emailsRemaining == 0) {
        deleteLater();
    }
}
