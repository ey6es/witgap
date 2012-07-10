//
// $Id$

#ifndef RESOURCE_CHOOSER_DIALOG
#define RESOURCE_CHOOSER_DIALOG

#include "ui/Button.h"
#include "ui/Window.h"
#include "util/General.h"

class ScrollingList;
class TextField;

/**
 * Base class for resource chooser dialogs.
 */
class ResourceChooserDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     *
     * @param allowZero whether or not we allow a zero selection.
     */
    ResourceChooserDialog (Session* parent, quint32 id = 0, bool allowZero = true);

signals:

    /**
     * Fired when a resource has been chosen.
     */
    void resourceChosen (const ResourceDescriptor& desc);

protected slots:

    /**
     * Updates the selected field based on the entered name.
     */
    void updateSelection ();

    /**
     * Updates the ok button based on the selection.
     */
    void updateOk ();

    /**
     * Fires the resource chosen signal with the selected descriptor.
     */
    void emitResourceChosen ();

protected:

    /**
     * Populates the list with the supplied descriptors.
     */
    Q_INVOKABLE void populateList (const ResourceDescriptorList& resources);

    /** The name field. */
    TextField* _name;

    /** The list of names. */
    ScrollingList* _list;

    /** The OK button. */
    Button* _ok;

    /** The id to select after list population. */
    quint32 _initialId;

    /** Whether or not we allow a zero id. */
    bool _allowZero;

    /** The resources corresponding to the names. */
    ResourceDescriptorList _resources;
};

/**
 * Chooser for zone ids.
 */
class ZoneChooserDialog : public ResourceChooserDialog
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    ZoneChooserDialog (Session* parent, quint32 id = 0, bool allowZero = true);
};

/**
 * Chooser for scene ids.
 */
class SceneChooserDialog : public ResourceChooserDialog
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    SceneChooserDialog (Session* parent, quint32 id = 0, bool allowZero = true);
};

/**
 * A button that brings up a resource chooser dialog.
 */
class ResourceChooserButton : public Button
{
    Q_OBJECT

public:

    /**
     * Initializes the button.
     */
    ResourceChooserButton (QObject* parent = 0);

    /**
     * Sets the resource id.
     */
    void setId (quint32 id);

    /**
     * Returns the currently selected id.
     */
    quint32 id () const { return _id; }

signals:

    /**
     * Fired when the selected id has changed.
     */
    void idChanged (quint32 id);

protected slots:

    /**
     * Opens the dialog to change the id.
     */
    virtual void openDialog () = 0;

    /**
     * Applies a change selected in the dialog.
     */
    void setValue (const ResourceDescriptor& value);

protected:

    /**
     * Loads from the database the name corresponding to the current id, calling back to
     * updateLabel.
     */
    virtual void loadName (Session* session) = 0;

    /**
     * Sets the label with the specified id and name.
     */
    Q_INVOKABLE void updateLabel (const QString& name);

    /** The currently selected id. */
    quint32 _id;
};

/**
 * Chooser button for zones.
 */
class ZoneChooserButton : public ResourceChooserButton
{
    Q_OBJECT

public:

    ZoneChooserButton (quint32 id = 0, QObject* parent = 0);

protected:

    /**
     * Opens the dialog to change the id.
     */
    virtual void openDialog ();

    /**
     * Loads from the database the name corresponding to the current id, calling back to
     * updateLabel.
     */
    virtual void loadName (Session* session);
};

/**
 * Chooser button for scenes.
 */
class SceneChooserButton : public ResourceChooserButton
{
    Q_OBJECT

public:

    SceneChooserButton (quint32 id = 0, QObject* parent = 0);

protected:

    /**
     * Opens the dialog to change the id.
     */
    virtual void openDialog ();

    /**
     * Loads from the database the name corresponding to the current id, calling back to
     * updateLabel.
     */
    virtual void loadName (Session* session);
};

#endif // RESOURCE_CHOOSER_DIALOG
