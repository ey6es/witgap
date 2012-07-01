//
// $Id$

#ifndef ID_CHOOSER_DIALOG
#define ID_CHOOSER_DIALOG

#include "ui/Window.h"
#include "util/General.h"

class Button;
class ScrollingList;
class TextField;

/**
 * Base class for id chooser dialogs.
 */
class IdChooserDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     *
     * @param allowZero whether or not we allow a zero selection.
     */
    IdChooserDialog (Session* parent, quint32 id = 0, bool allowZero = true);

signals:

    /**
     * Fired when an id has been selected.
     */
    void idSelected (quint32 id, const QString& name);

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
     * Fires the item selected signal with the selected id and name.
     */
    void emitIdSelected ();

protected:

    /**
     * Populates the list with the supplied descriptors.
     */
    Q_INVOKABLE void populateList (const DescriptorList& values);

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
};

/**
 * Chooser for zone ids.
 */
class ZoneIdChooserDialog : public IdChooserDialog
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    ZoneIdChooserDialog (Session* parent, quint32 id = 0, bool allowZero = true);
};

/**
 * Chooser for scene ids.
 */
class SceneIdChooserDialog : public IdChooserDialog
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    SceneIdChooserDialog (Session* parent, quint32 id = 0, bool allowZero = true);
};

#endif // ID_CHOOSER_DIALOG
