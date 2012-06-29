//
// $Id$

#ifndef OBJECT_EDITOR
#define OBJECT_EDITOR

#include <QMetaProperty>

#include "ui/Component.h"

class Button;
class CheckBox;
class ComboBox;
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
     *
     * @param propertyOffset the index of the first property to allow editing.
     */
    ObjectEditor (QObject* object = 0, int propertyOffset = 0, QObject* parent = 0);

    /**
     * Sets the object to edit.
     *
     * @param propertyOffset the index of the first property to allow editing.
     */
    void setObject (QObject* object, int propertyOffset = 0);

    /**
     * Returns a pointer to the object being edited.
     */
    QObject* object () const { return _object; }

    /**
     * Updates all property editors.
     */
    void update ();

    /**
     * Applies all property editors.
     */
    void apply ();

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

    /**
     * Applies a change to the object.
     */
    virtual void apply () { }

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

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

    /**
     * Applies a change to the object.
     */
    virtual void apply ();

protected:

    /** The checkbox. */
    CheckBox* _box;
};

/**
 * Uses a combo box to edit an enum property.
 */
class EnumPropertyEditor : public PropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    EnumPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

    /**
     * Applies a change to the object.
     */
    virtual void apply ();

protected:

    /** The combo box. */
    ComboBox* _box;
};

/**
 * Uses a set of checkboxes to edit a set of flags.
 */
class FlagPropertyEditor : public PropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    FlagPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

    /**
     * Applies a change to the object.
     */
    virtual void apply ();
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

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

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

    /**
     * Applies a change to the object.
     */
    virtual void apply ();
};

/**
 * Base class for ZoneIdPropertyEditor and SceneIdPropertyEditor.
 */
class AbstractIdPropertyEditor : public PropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    AbstractIdPropertyEditor (const QMetaProperty& property, QObject* parent = 0);

    /**
     * Updates the edited value in response to an external change.
     */
    virtual void update ();

protected slots:

    /**
     * Opens the dialog to change the id.
     */
    virtual void openDialog () = 0;

protected:

    /**
     * Loads from the database the name corresponding to the provided id, calling back to
     * setButtonLabel.
     */
    virtual void loadName (Session* session, quint32 id) = 0;

    /**
     * Sets the button label with the specified id and name.
     */
    Q_INVOKABLE void setButtonLabel (quint32 id, const QString& name);

    /** The button that brings up the dialog. */
    Button* _button;
};

/**
 * Edits a zone id property.
 */
class ZoneIdPropertyEditor : public AbstractIdPropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    ZoneIdPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

protected:

    /**
     * Opens the dialog to change the id.
     */
    virtual void openDialog ();

    /**
     * Loads from the database the name corresponding to the provided id, calling back to
     * setButtonLabel.
     */
    virtual void loadName (Session* session, quint32 id);
};

/**
 * Edits a scene id property.
 */
class SceneIdPropertyEditor : public AbstractIdPropertyEditor
{
    Q_OBJECT

public:

    /**
     * Initializes the editor.
     */
    SceneIdPropertyEditor (QObject* object, const QMetaProperty& property, QObject* parent = 0);

protected:

    /**
     * Opens the dialog to change the id.
     */
    virtual void openDialog ();

    /**
     * Loads from the database the name corresponding to the provided id, calling back to
     * setButtonLabel.
     */
    virtual void loadName (Session* session, quint32 id);
};

#endif // OBJECT_EDITOR
