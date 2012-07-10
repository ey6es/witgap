//
// $Id$

#include <QMetaObject>
#include <QTranslator>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Layout.h"
#include "ui/ResourceChooserDialog.h"
#include "ui/ScrollingList.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("ResourceChooserDialog", __VA_ARGS__)

ResourceChooserDialog::ResourceChooserDialog (Session* parent, quint32 id, bool allowZero) :
    Window(parent, parent->highestWindowLayer(), true, true),
    _initialId(id),
    _allowZero(allowZero)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* ncont = BoxLayout::createHStretchBox(1);
    addChild(ncont);
    ncont->addChild(new Label(tr("Name:")), BoxLayout::Fixed);
    ncont->addChild(_name = new TextField(20, new Document("", 255)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateSelection()));

    addChild(_list = new ScrollingList());
    if (!allowZero) {
        connect(_list, SIGNAL(selectionChanged()), SLOT(updateOk()));
    }

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    _ok = new Button(tr("OK"));
    _ok->setEnabled(false);
    _ok->connect(_name, SIGNAL(enterPressed()), SLOT(doPress()));
    _ok->connect(_list, SIGNAL(enterPressed()), SLOT(doPress()));
    connect(_ok, SIGNAL(pressed()), SLOT(emitResourceChosen()));
    connect(_ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _ok));

    pack();
    center();
}

void ResourceChooserDialog::updateSelection ()
{
    QString prefix = _name->text().simplified();
    for (int ii = 0, nn = _list->values().length(); ii < nn; ii++) {
        if (_list->values().at(ii).toLower().startsWith(prefix, Qt::CaseInsensitive)) {
            _list->setSelectedIndex(ii);
            return;
        }
    }
    _list->setSelectedIndex(-1);
}

void ResourceChooserDialog::updateOk ()
{
    _ok->setEnabled(_allowZero || _list->selectedIndex() != -1);
}

void ResourceChooserDialog::emitResourceChosen ()
{
    int idx = _list->selectedIndex();
    emit resourceChosen(idx == -1 ? NoResource : _resources.at(idx));
}

void ResourceChooserDialog::populateList (const ResourceDescriptorList& resources)
{
    _resources = resources;

    QStringList names;
    int idx = -1;
    for (int ii = 0, nn = resources.size(); ii < nn; ii++) {
        const ResourceDescriptor& resource = resources.at(ii);
        names.append(resource.name);
        if (resource.id == _initialId) {
            idx = ii;
        }
    }
    _list->setValues(names);

    if (idx != -1) {
        _list->setSelectedIndex(idx);
    }
    updateOk();
}

ZoneChooserDialog::ZoneChooserDialog (Session* parent, quint32 id, bool allowZero) :
    ResourceChooserDialog(parent, id, allowZero)
{
    QMetaObject::invokeMethod(parent->app()->databaseThread()->sceneRepository(), "findZones",
        Q_ARG(const QString&, ""), Q_ARG(quint32, 0), Q_ARG(const Callback&, Callback(_this,
            "populateList(ResourceDescriptorList)")));
}

SceneChooserDialog::SceneChooserDialog (Session* parent, quint32 id, bool allowZero) :
    ResourceChooserDialog(parent, id, allowZero)
{
    QMetaObject::invokeMethod(parent->app()->databaseThread()->sceneRepository(), "findScenes",
        Q_ARG(const QString&, ""), Q_ARG(quint32, 0), Q_ARG(const Callback&, Callback(_this,
            "populateList(ResourceDescriptorList)")));
}

ResourceChooserButton::ResourceChooserButton (QObject* parent) :
    Button(QString(), Qt::AlignLeft, parent),
    _id(-1)
{
    connect(this, SIGNAL(pressed()), SLOT(openDialog()));
}

void ResourceChooserButton::setId (quint32 id)
{
    if (_id != id) {
        if ((_id = id) == 0) {
            updateLabel("");
            return;
        }
        setLabel(QString::number(_id));

        Session* session = this->session();
        if (session != 0) {
            loadName(session);
        }
    }
}

void ResourceChooserButton::setValue (const ResourceDescriptor& value)
{
    if (_id != value.id) {
        _id = value.id;
        updateLabel(value.name);

        emit idChanged(_id);
    }
}

void ResourceChooserButton::updateLabel (const QString& name)
{
    setLabel(_id == 0 ? "---" : QString::number(_id) + (": " + name));
}

ZoneChooserButton::ZoneChooserButton (quint32 id, QObject* parent) :
    ResourceChooserButton(parent)
{
    setId(id);
}

SceneChooserButton::SceneChooserButton (quint32 id, QObject* parent) :
    ResourceChooserButton(parent)
{
    setId(id);
}

void ZoneChooserButton::openDialog ()
{
    ZoneChooserDialog* dialog = new ZoneChooserDialog(session(), _id);
    connect(dialog, SIGNAL(resourceChosen(ResourceDescriptor)),
        SLOT(setValue(ResourceDescriptor)));
}

void ZoneChooserButton::loadName (Session* session)
{
    QMetaObject::invokeMethod(session->app()->databaseThread()->sceneRepository(), "loadZoneName",
        Q_ARG(quint32, _id), Q_ARG(const Callback&, Callback(_this, "updateLabel(QString)")));
}

void SceneChooserButton::openDialog ()
{
    SceneChooserDialog* dialog = new SceneChooserDialog(session(), _id);
    connect(dialog, SIGNAL(resourceChosen(ResourceDescriptor)),
        SLOT(setValue(ResourceDescriptor)));
}

void SceneChooserButton::loadName (Session* session)
{
    QMetaObject::invokeMethod(session->app()->databaseThread()->sceneRepository(), "loadSceneName",
        Q_ARG(quint32, _id), Q_ARG(const Callback&, Callback(_this, "updateLabel(QString)")));
}
