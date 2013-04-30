//
// $Id$

#include "ui/Button.h"
#include "ui/Layout.h"
#include "ui/TabbedPane.h"

TabbedPane::TabbedPane (Qt::Orientation orientation, QObject* parent) :
    Container(0, parent)
{
    if (orientation == Qt::Vertical) {
        setLayout(new BoxLayout(Qt::Vertical,
            BoxLayout::HStretch | BoxLayout::VStretch, Qt::AlignCenter, 0));
        addChild(_buttons = BoxLayout::createHBox(Qt::AlignLeft), BoxLayout::Fixed);
    
    } else {
        setLayout(new BoxLayout(Qt::Horizontal,
            BoxLayout::HStretch | BoxLayout::VStretch, Qt::AlignCenter, 1));
        addChild(_buttons = BoxLayout::createVBox(Qt::AlignTop), BoxLayout::Fixed);
    }
}

void TabbedPane::addTab (const QString& title, Component* comp)
{
    Button* button = new Button(title);
    button->setProperty("component", QVariant::fromValue<QObject*>(comp));
    comp->setParent(this);
    connect(button, SIGNAL(pressed()), SLOT(switchToSenderTab()));
    _buttons->addChild(button);
    
    if (_buttons->visibleChildCount() == 1) {
        addChild(comp);
    }
}

void TabbedPane::switchToSenderTab ()
{
    Component* comp = static_cast<Component*>(sender()->property("component").value<QObject*>());
    removeChildAt(1, false);
    addChild(comp);
}
