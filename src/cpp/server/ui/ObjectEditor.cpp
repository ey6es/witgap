//
// $Id$

#include <QtDebug>

#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/ObjectEditor.h"
#include "ui/TextField.h"

ObjectEditor::ObjectEditor (QObject* object, int propertyOffset, QObject* parent) :
    Container(new TableLayout(2), parent),
    _object(0)
{
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
        PropertyEditor* editor = PropertyEditor::create(object, property);
        if (editor == 0) {
            continue;
        }
        addChild(new Label(property.name()));
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
    switch (property.type()) {
        case QVariant::Bool:
            return new BoolPropertyEditor(object, property, parent);

        case QVariant::String:
            return new StringPropertyEditor(object, property, parent);
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
        // the '2' identified the method as a signal (see definition for SIGNAL macro)
        connect(object, QByteArray(_property.notifySignal().signature()).prepend('2'),
            SLOT(update()));
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
