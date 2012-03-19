//
// $Id$

#include <limits>

#include "net/Session.h"
#include "scene/EditSceneDialog.h"
#include "scene/Scene.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"
#include "util/Callback.h"

// translate through the session
#define tr(...) session()->translate("EditSceneDialog", __VA_ARGS__)

using namespace std;

/** Expression for unsigned shorts. */
const QRegExp UShortExp("\\d{0,5}");

EditSceneDialog::EditSceneDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* icont = new Container(new TableLayout(2));
    icont->setBorder(new CharBorder(QMargins(1, 0, 1, 0), 0));
    addChild(icont);

    const SceneRecord& record = parent->scene()->record();

    icont->addChild(new Label(tr("ID:")));
    icont->addChild(new Label(QString::number(record.id)));

    icont->addChild(new Label(tr("Creator:")));
    icont->addChild(new Label(record.creatorName));

    icont->addChild(new Label(tr("Created:")));
    icont->addChild(new Label(record.created.toString()));

    icont->addChild(new Spacer(1, 1));
    icont->addChild(new Spacer(1, 1));

    icont->addChild(new Label(tr("Name:")));
    icont->addChild(_name = new TextField(20, new Document(record.name, 255)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateUpdate()));

    icont->addChild(new Label(tr("Scroll Width:")));
    icont->addChild(_scrollWidth = new TextField(20,
        new RegExpDocument(UShortExp, QString::number(record.scrollWidth), 5)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateUpdate()));

    icont->addChild(new Label(tr("Scroll Height:")));
    icont->addChild(_scrollHeight = new TextField(20,
        new RegExpDocument(UShortExp, QString::number(record.scrollHeight), 5)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateUpdate()));

    Button* close = new Button(tr("Close"));
    connect(close, SIGNAL(pressed()), SLOT(deleteLater()));
    Button* del = new Button(tr("Delete"));
    connect(del, SIGNAL(pressed()), SLOT(deleteLater()));
    connect(_update = new Button(tr("Update")), SIGNAL(pressed()), SLOT(update()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, close, del, _update));
}

void EditSceneDialog::updateUpdate ()
{
    int scrollWidth = _scrollWidth->text().toInt();
    int scrollHeight = _scrollHeight->text().toInt();
    _update->setEnabled(!_name->text().trimmed().isEmpty() &&
        scrollWidth > 0 && scrollWidth < numeric_limits<quint16>::max() &&
        scrollHeight > 0 && scrollHeight < numeric_limits<quint16>::max());
}

void EditSceneDialog::confirmDelete ()
{
    session()->showConfirmDialog(tr("Are you sure you want to delete this scene?"),
        Callback(_this, "reallyDelete()"));
}

void EditSceneDialog::update ()
{
    // handle update through the scene
    session()->scene()->update(_name->text().simplified(),
        _scrollWidth->text().toInt(), _scrollHeight->text().toInt());
}

void EditSceneDialog::reallyDelete ()
{
    // handle deletion through the scene
    session()->scene()->remove();
}
