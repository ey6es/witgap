//
// $Id$

#ifndef TABBED_PANE
#define TABBED_PANE

#include "ui/Component.h"

class ButtonGroup;

/**
 * A tabbed pane component.
 */
class TabbedPane : public Container
{
    Q_OBJECT

public:
    
    /**
     * Creates a new tabbed pane.
     */
    TabbedPane (Qt::Orientation orientation = Qt::Vertical, QObject* parent = 0);
    
    /**
     * Adds a tab to the pane.
     */
    void addTab (const QString& title, Component* comp);

protected slots:
    
    /**
     * Switches to the sending button's tab.
     */
    void switchToSenderTab ();    
    
protected:

    /** Holds the tab buttons. */
    Container* _buttons;
    
    /** Holds the tabs. */
    Container* _tabs;
    
    /** Handles toggle button switching behavior. */
    ButtonGroup* _group;
};

#endif // TABBED_PANE
