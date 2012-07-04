//
// $Id$

#include <QMetaObject>
#include <QTranslator>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "net/Session.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/ObjectEditor.h"
#include "ui/ResourceChooserDialog.h"
#include "ui/TextField.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("ObjectEditor", __VA_ARGS__)

ObjectEditor::ObjectEditor (QObject* object, int propertyOffset, QObject* parent) :
    Container(new TableLayout(2), parent),
    _object(0)
{
    // allow the value column to stretch
    static_cast<TableLayout*>(_layout)->stretchColumns().insert(1);

    setObject(object, propertyOffset);
}

void ObjectEditor::setObject (QObject* object, int propertyOffset)
{
    if (_object == object) {
        return;
    }
    removeAllChildren();
    if ((_object = object) == 0) {
        return;
    }
    const QMetaObject* meta = _object->metaObject();
    for (int ii = propertyOffset, nn = meta->propertyCount(); ii < nn; ii++) {
        QMetaProperty property = meta->property(ii);
        PropertyEditor* editor = PropertyEditor::create(object, property, this);
        if (editor == 0) {
            continue;
        }
        addChild(new Label(QString(property.name()) + ":"));
        addChild(editor);
    }
}

void ObjectEditor::update ()
{
    for (int ii = 1, nn = _children.size(); ii < nn; ii += 2) {
        static_cast<PropertyEditor*>(_children.at(ii))->update();
    }
}

void ObjectEditor::apply ()
{
    for (int ii = 1, nn = _children.size(); ii < nn; ii += 2) {
        static_cast<PropertyEditor*>(_children.at(ii))->apply();
    }
}

PropertyEditor* PropertyEditor::create (
    QObject* object, const QMetaProperty& property, QObject* parent)
{
    if (!property.isWritable()) {
        return new ReadOnlyPropertyEditor(object, property, parent);
    }
    if (property.isFlagType()) {
        return new FlagPropertyEditor(object, property, parent);
    }
    if (property.isEnumType()) {
        return new EnumPropertyEditor(object, property, parent);
    }
    QByteArray name(property.name());
    switch (property.type()) {
        case QVariant::Bool:
            return new BoolPropertyEditor(object, property, parent);

        case QVariant::String:
            return new StringPropertyEditor(object, property, parent);

        case QVariant::UInt:
            if (name.endsWith("Zone")) {
                return new ZoneIdPropertyEditor(object, property, parent);

            } else if (name.endsWith("Scene")) {
                return new SceneIdPropertyEditor(object, property, parent);
            }
            break;
    }
    qWarning() << "Don't know how to edit property." << object << property.name();
    return 0;
}

PropertyEditor::PropertyEditor (const QMetaProperty& property, QObject* parent) :
    Container(new BoxLayout(Qt::Horizontal, BoxLayout::HStretch), parent),
    _object(0),
    _property(property)
{
}

void PropertyEditor::setObject (QObject* object)
{
    if (_object == object) {
        return;
    }
    if (_property.hasNotifySignal()) {
        if (_object != 0) {
            _object->disconnect(this);
        }
        connect(object, signal(_property.notifySignal().signature()), SLOT(update()));
    }
    _object = object;
    update();
}

ReadOnlyPropertyEditor::ReadOnlyPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    PropertyEditor(property, parent),
    _label(new Label())
{
    addChild(_label);
    setObject(object);
}

void ReadOnlyPropertyEditor::update ()
{
    _label->setText(_property.read(_object).toString());
}

BoolPropertyEditor::BoolPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    PropertyEditor(property, parent),
    _box(new CheckBox())
{
    addChild(_box);
    connect(_box, SIGNAL(pressed()), SLOT(apply()));

    setObject(object);
}

void BoolPropertyEditor::update ()
{
    _box->setSelected(_property.read(_object).toBool());
}

void BoolPropertyEditor::apply ()
{
    _property.write(_object, _box->selected());
}

EnumPropertyEditor::EnumPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    PropertyEditor(property, parent)
{
    QStringList items;
    QMetaEnum enumerator = property.enumerator();
    for (int ii = 0, nn = enumerator.keyCount(); ii < nn; ii++) {
        items.append(enumerator.key(ii));
    }
    addChild(_box = new ComboBox(items));
    connect(_box, SIGNAL(selectionChanged()), SLOT(apply()));

    setObject(object);
}

void EnumPropertyEditor::update ()
{
    int value = _property.read(_object).toInt();
    QMetaEnum enumerator = _property.enumerator();
    for (int ii = 0, nn = enumerator.keyCount(); ii < nn; ii++) {
        if (enumerator.value(ii) == value) {
            _box->setSelectedIndex(ii);
            return;
        }
    }
}

void EnumPropertyEditor::apply ()
{
    _property.write(_object, _property.enumerator().value(_box->selectedIndex()));
}

FlagPropertyEditor::FlagPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    PropertyEditor(property, parent)
{
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 0));

    QMetaEnum enumerator = property.enumerator();
    for (int ii = 0, nn = enumerator.keyCount(); ii < nn; ii++) {
        CheckBox* box = new CheckBox(enumerator.key(ii));
        connect(box, SIGNAL(pressed()), SLOT(apply()));
        addChild(box);
    }

    setObject(object);
}

void FlagPropertyEditor::update ()
{
    QMetaEnum enumerator = _property.enumerator();
    int value = _property.read(_object).toInt();
    for (int ii = 0, nn = enumerator.keyCount(); ii < nn; ii++) {
        static_cast<CheckBox*>(_children.at(ii))->setSelected((value & enumerator.value(ii)) != 0);
    }
}

void FlagPropertyEditor::apply ()
{
    QMetaEnum enumerator = _property.enumerator();
    int value = 0;
    for (int ii = 0, nn = enumerator.keyCount(); ii < nn; ii++) {
        if (static_cast<CheckBox*>(_children.at(ii))->selected()) {
            value |= enumerator.value(ii);
        }
    }
    _property.write(_object, value);
}

AbstractStringPropertyEditor::AbstractStringPropertyEditor (
        const QMetaProperty& property, TextField* field, QObject* parent) :
    PropertyEditor(property, parent),
    _field(field)
{
    addChild(_field);
    connect(_field, SIGNAL(enterPressed()), SLOT(apply()));
}

void AbstractStringPropertyEditor::update ()
{
    _field->setText(_property.read(_object).toString());
}

StringPropertyEditor::StringPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    AbstractStringPropertyEditor(property, new TextField(), parent)
{
    setObject(object);
}

void StringPropertyEditor::apply ()
{
    _property.write(_object, _field->text());
}

ResourceIdPropertyEditor::ResourceIdPropertyEditor (
        const QMetaProperty& property, QObject* parent) :
    PropertyEditor(property, parent),
    _button(new Button())
{
    addChild(_button);
    connect(_button, SIGNAL(pressed()), SLOT(openDialog()));
}

void ResourceIdPropertyEditor::update ()
{
    quint32 id = _property.read(_object).toUInt();
    if (id == 0) {
        setButtonLabel(0, "");
        return;
    }
    _button->setLabel(QString::number(id));

    // if we have the session, load the name from the database
    Session* session = this->session();
    if (session != 0) {
        loadName(session, id);
    }
}

void ResourceIdPropertyEditor::setValue (const ResourceDescriptor& value)
{
    _property.write(_object, value.id);
    setButtonLabel(value.id, value.name);
}

void ResourceIdPropertyEditor::setButtonLabel (quint32 id, const QString& name)
{
    _button->setLabel(id == 0 ? "---" : QString::number(id) + (": " + name));
}

ZoneIdPropertyEditor::ZoneIdPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    ResourceIdPropertyEditor(property, parent)
{
    setObject(object);
}

void ZoneIdPropertyEditor::openDialog ()
{
    ZoneChooserDialog* dialog = new ZoneChooserDialog(session(), _property.read(_object).toUInt());
    connect(dialog, SIGNAL(resourceChosen(ResourceDescriptor)),
        SLOT(setValue(ResourceDescriptor)));
}

void ZoneIdPropertyEditor::loadName (Session* session, quint32 id)
{
    QMetaObject::invokeMethod(session->app()->databaseThread()->sceneRepository(), "loadZoneName",
        Q_ARG(quint32, id), Q_ARG(const Callback&, Callback(
            _this, "setButtonLabel(quint32,QString)", Q_ARG(quint32, id))));
}

SceneIdPropertyEditor::SceneIdPropertyEditor (
        QObject* object, const QMetaProperty& property, QObject* parent) :
    ResourceIdPropertyEditor(property, parent)
{
    setObject(object);
}

void SceneIdPropertyEditor::openDialog ()
{
    SceneChooserDialog* dialog = new SceneChooserDialog(
        session(), _property.read(_object).toUInt());
    connect(dialog, SIGNAL(resourceChosen(ResourceDescriptor)),
        SLOT(setValue(ResourceDescriptor)));
}

void SceneIdPropertyEditor::loadName (Session* session, quint32 id)
{
    QMetaObject::invokeMethod(session->app()->databaseThread()->sceneRepository(), "loadSceneName",
        Q_ARG(quint32, id), Q_ARG(const Callback&, Callback(
            _this, "setButtonLabel(quint32,QString)", Q_ARG(quint32, id))));
}
