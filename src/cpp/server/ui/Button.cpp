//
// $Id$

#include <QKeyEvent>
#include <QMouseEvent>

#include "Protocol.h"
#include "ui/Button.h"
#include "ui/Layout.h"
#include "ui/Menu.h"

Button::Button (const QString& label, Qt::Alignment alignment, QObject* parent) :
    Label(QIntVector(), alignment, parent),
    _label(label)
{
    updateText();
    updateMargins();
}

void Button::setLabel (const QString& label)
{
    if (_label != label) {
        _label = label;
        updateText();
    }
}

void Button::doPress ()
{
    if (!_enabled) {
        return;
    }
    if (!_focused) {
        requestFocus();
    }
    emit pressed();
}

void Button::updateMargins ()
{
    Label::updateMargins();

    // add space for brackets
    _margins.setLeft(_margins.left() + 1);
    _margins.setRight(_margins.right() + 1);
}

void Button::draw (DrawContext* ctx)
{
    Label::draw(ctx);

    // draw the brackets
    int flags = _enabled ? 0 : DIM_FLAG;
    ctx->drawChar(_margins.left() - 1, _margins.top(), '[' | flags);
    ctx->drawChar(_bounds.width() - _margins.right(), _margins.top(), ']' | flags);
}

void Button::focusInEvent (QFocusEvent* e)
{
    updateText();
}

void Button::focusOutEvent (QFocusEvent* e)
{
    updateText();
}

void Button::mouseButtonReleaseEvent (QMouseEvent* e)
{
    if (localBounds().contains(e->pos())) {
        emit pressed();
    }
}

void Button::keyPressEvent (QKeyEvent* e)
{
    int key = e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if ((key == Qt::Key_Return || key == Qt::Key_Enter) &&
            (modifiers == Qt::ShiftModifier || modifiers == Qt::NoModifier)) {
        emit pressed();

    } else {
        Label::keyPressEvent(e);
    }
}

void Button::updateText ()
{
    QIntVector text = QIntVector::createHighlighted(_label);
    if (_focused) {
        for (int* ptr = text.data(), *end = ptr + text.size(); ptr < end; ptr++) {
            *ptr |= REVERSE_FLAG;
        }
    }
    setText(text);
}

CheckBox::CheckBox (
        const QString& label, bool selected, Qt::Alignment alignment, QObject* parent) :
    Button(label, alignment, parent),
    _selected(selected)
{
    connect(this, SIGNAL(pressed()), SLOT(toggleSelected()));
    updateMargins();
}

void CheckBox::setSelected (bool selected)
{
    if (_selected != selected) {
        _selected = selected;
        dirty(QRect(_margins.left() - 3, _margins.top(), 1, 1));
    }
}

void CheckBox::updateMargins ()
{
    Label::updateMargins();

    // add space for check indicator
    _margins.setLeft(_margins.left() + 4);
}

void CheckBox::draw (DrawContext* ctx)
{
    Label::draw(ctx);

    // draw the indicator
    int flags = _enabled ? 0 : DIM_FLAG;
    ctx->drawChar(_margins.left() - 4, _margins.top(), '[' | flags);
    ctx->drawChar(_margins.left() - 3, _margins.top(),
        (_selected ? 'X' : ' ') | (_focused ? REVERSE_FLAG : 0) | flags);
    ctx->drawChar(_margins.left() - 2, _margins.top(), ']' | flags);
}

void CheckBox::focusInEvent (QFocusEvent* e)
{
    dirty(QRect(_margins.left() - 3, _margins.top(), 1, 1));
}

void CheckBox::focusOutEvent (QFocusEvent* e)
{
    dirty(QRect(_margins.left() - 3, _margins.top(), 1, 1));
}

void CheckBox::updateText ()
{
    setText(QIntVector::createHighlighted(_label));
}

ComboBox::ComboBox (const QStringList& items, Qt::Alignment alignment, QObject* parent) :
    Button(QString(), alignment, parent),
    _selectedIndex(0)
{
    connect(this, SIGNAL(pressed()), SLOT(createMenu()));
    setItems(items);
}

void ComboBox::setItems (const QStringList& items)
{
    if (_items != items) {
        _items = items;
        _selectedIndex = qBound(-1, _selectedIndex, _items.size() - 1);
        setLabel(selectedItem());
    }
}

void ComboBox::setSelectedIndex (int index)
{
    if (_selectedIndex != index) {
        _selectedIndex = index;
        setLabel(selectedItem());
    }
}

QString ComboBox::selectedItem () const
{
    return _selectedIndex >= 0 && _selectedIndex < _items.size() ?
        _items.at(_selectedIndex) : QString();
}

void ComboBox::createMenu ()
{
    if (_items.isEmpty()) {
        return;
    }
    _menu = new Menu(session());
    _menu->setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 0));
    _menu->connect(this, SIGNAL(destroyed()), SLOT(deleteLater()));
    foreach (const QString& item, _items) {
        _menu->addButton(item, this, SLOT(selectItem()));
    }
    _menu->pack();
    _menu->setBounds(QRect(absolutePos() - QPoint(1, 1),
        _menu->preferredSize(_bounds.width() + 2, -1)));
    if (_selectedIndex >= 0 && _selectedIndex <= _items.size()) {
        _menu->children().at(_selectedIndex)->requestFocus();
    }
}

void ComboBox::selectItem ()
{
    Component* button = static_cast<Component*>(sender());
    int index = _menu->children().indexOf(button);
    if (_selectedIndex != index) {
        setSelectedIndex(index);
        emit selectionChanged();
    }
}

QSize ComboBox::computePreferredSize (int whint, int hhint) const
{
    int width = qMax(whint, 0);
    foreach (const QString& item, _items) {
        width = qMax(item.length(), width);
    }
    return QSize(width, 1);
}
