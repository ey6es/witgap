//
// $Id$

#ifndef RESOURCE_CHOOSER_DIALOG
#define RESOURCE_CHOOSER_DIALOG

#include "ui/Window.h"
#include "util/General.h"

class Button;
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

#endif // RESOURCE_CHOOSER_DIALOG
