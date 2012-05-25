//
// $Id$

#ifndef OBJECT_EDITOR
#define OBJECT_EDITOR

#include <QMetaProperty>

#include "ui/Component.h"

class CheckBox;
class Label;
class TextField;

/**
 * A component that allows editing the Qt properties of an object.
 */
class ObjectEditor : public Container
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    ObjectEditor (QObject* object = 0, QObject* parent = 0);

    /**
     * Sets the object to edit.
     */
    void setObject (QObject* object);

    /**
     * Returns a pointer to the object being edited.
     */
    QObject* object () const { return _object; }

    /**
     * Updates all property editors.
     */
    void update ();

protected:

    /** The object we're editing. */
    QObject* _object;
};

/**
 * Edits a single object property.
 */
class PropertyEditor : public Container
{
    Q_OBJECT

public:

    /**
     * Creates an editor for the supplied property.
     */
    static PropertyEditor* create (
        QObject* object, const QMetaProperty& property, QObject* parent = 0);

    /**
     * Initializes the editor.
     */
    PropertyEditor (const QMetaProperty& property, QObject* parent = 0);

    /**
     * Sets the object to edit.
     */
    void setObject (QObject* object);

public slots:

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update () = 0;

protected:

    /** The object that we're editing. */
    QObject* _object;

    /** The property we edit. */
    QMetaProperty _property;
};

/**
 * Displays a read-only property by converting it to a string.
 */
class ReadOnlyPropertyEditor : public PropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    ReadOnlyPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

public slots:

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

protected:

    /** The label displaying the value. */
    Label* _label;
};

/**
 * Edits a boolean property by means of a checkbox.
 */
class BoolPropertyEditor : public PropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    BoolPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

public slots:

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

protected slots:

    /**
     * Applies a change to the object.
     */
    void apply ();

protected:

    /** The checkbox. */
    CheckBox* _box;
};

/**
 * Base class for editors that use a text field.
 */
class AbstractStringPropertyEditor : public PropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    AbstractStringPropertyEditor (
        const QMetaProperty& property, TextField* field, QObject* parent = 0);

public slots:

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

protected slots:

    /**
     * Applies a change to the object.
     */
    virtual void apply () = 0;

protected:

    /** The text field. */
    TextField* _field;
};

/**
 * Uses a text field to edit a string property.
 */
class StringPropertyEditor : public AbstractStringPropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    StringPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

protected slots:

    /**
     * Applies a change to the object.
     */
    virtual void apply ();
};

#endif // OBJECT_EDITOR
