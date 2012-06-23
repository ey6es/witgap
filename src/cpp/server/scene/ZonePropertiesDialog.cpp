//
// $Id$

#include <QTranslator>

#include <limits>

#include "net/Session.h"
#include "scene/Zone.h"
#include "scene/ZonePropertiesDialog.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"
#include "util/Callback.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("ZonePropertiesDialog", __VA_ARGS__)

using namespace std;

/** Expression for unsigned shorts. */
const QRegExp UShortExp("\\d{0,5}");

ZonePropertiesDialog::ZonePropertiesDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* icont = new Container(new TableLayout(2));
    icont->setBorder(new CharBorder(QMargins(1, 0, 1, 0), 0));
    addChild(icont);

    Instance* instance = parent->instance();
    connect(instance, SIGNAL(recordChanged(ZoneRecord)), SLOT(update()));
    const ZoneRecord& record = instance->record();

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
    connect(_name, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Max. Population:")));
    icont->addChild(_maxPopulation = new TextField(20,
        new RegExpDocument(UShortExp, QString::number(record.maxPopulation), 5), true));
    connect(_name, SIGNAL(textChanged()), SLOT(updateApply()));

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    Button* del = new Button(tr("Delete"));
    connect(del, SIGNAL(pressed()), SLOT(confirmDelete()));
    connect(_apply = new Button(tr("Apply")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok = new Button(tr("OK")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, del, _apply, _ok));

    pack();
    center();
}

void ZonePropertiesDialog::update ()
{
    const ZoneRecord& record = session()->instance()->record();
    _name->setText(record.name);
    _maxPopulation->setText(QString::number(record.maxPopulation));
    updateApply();
}

void ZonePropertiesDialog::updateApply ()
{
    int maxPopulation = _maxPopulation->text().toInt();
    bool enable = !_name->text().trimmed().isEmpty() &&
        maxPopulation > 0 && maxPopulation < numeric_limits<quint16>::max();
    _apply->setEnabled(enable);
    _ok->setEnabled(enable);
}

void ZonePropertiesDialog::confirmDelete ()
{
    session()->showConfirmDialog(tr("Are you sure you want to delete this zone?"),
        Callback(_this, "reallyDelete()"));
}

void ZonePropertiesDialog::apply ()
{
    // handle update through the instance
    session()->instance()->setProperties(_name->text().simplified(),
        _maxPopulation->text().toInt());
}

void ZonePropertiesDialog::reallyDelete ()
{
    // handle deletion through the instance
    session()->instance()->remove();
}
