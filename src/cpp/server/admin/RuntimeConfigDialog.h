//
// $Id$

#ifndef RUNTIME_CONFIG_DIALOG
#define RUNTIME_CONFIG_DIALOG

#include "ui/Window.h"

class ObjectEditor;
class Session;

/**
 * Allows editing the runtime server configuration.
 */
class RuntimeConfigDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    Q_INVOKABLE RuntimeConfigDialog (Session* parent);

protected slots:

    /**
     * Applies the current state.
     */
    void apply ();

protected:

    /** The object editor. */
    ObjectEditor* _editor;
};

#endif // RUNTIME_CONFIG_DIALOG
