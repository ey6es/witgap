//
// $Id$

#ifndef CHOOSER_DIALOG
#define CHOOSER_DIALOG

#include "ui/Window.h"

class Button;
class ScrollingList;
class TextField;

/**
 * Base class for chooser dialogs.
 */
class ChooserDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     *
     * @param allowEmpty whether or not we allow an empty selection.
     */
    ChooserDialog (Session* parent, bool allowEmpty = true);

protected slots:

    /**
     * Updates the selected field based on the entered name.
     */
    void updateSelection ();

    /**
     * Updates the ok button based on the selection.
     */
    void updateOk ();

protected:

    /** The scene name field. */
    TextField* _name;

    /** The list of names. */
    ScrollingList* _list;

    /** The OK button. */
    Button* _ok;
};

/**
 * Chooser for zones.
 */
class ZoneChooserDialog : public ChooserDialog
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    ZoneChooserDialog (Session* parent, bool allowEmpty = true);
};

/**
 * Chooser for scenes.
 */
class SceneChooserDialog : public ChooserDialog
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    SceneChooserDialog (Session* parent, bool allowEmpty = true);
};

#endif // CHOOSER_DIALOG
