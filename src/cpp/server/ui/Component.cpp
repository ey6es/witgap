//
// $Id$

#include "ui/Border.h"
#include "ui/Component.h"
#include "ui/Layout.h"

Component::Component (QObject* parent) :
    QObject(parent),
    _border(0),
    _explicitPreferredSize(-1, -1),
    _background(' '),
    _valid(false)
{
    // connect slots
    connect(this, SIGNAL(boundsChanged()), SLOT(invalidate()));
}

Component::~Component ()
{
    if (_border != 0) {
        delete _border;
    }
}

void Component::setBounds (const QRect& bounds)
{
    if (_bounds != bounds) {
        _bounds = bounds;
        emit boundsChanged();
    }
}

void Component::setBorder (Border* border)
{
    if (_border != border) {
        if (_border != 0) {
            delete _border;
        }
        _border = border;
    }
}

void Component::setBackground (int background)
{
    _background = background;
}

void Component::setPreferredSize (const QSize& size)
{
    if (_explicitPreferredSize != size) {
        _explicitPreferredSize = size;
        invalidateParent();
    }
}

QSize Component::preferredSize (int whint, int hhint) const
{
    // the size may be entirely explicit
    int ewidth = _explicitPreferredSize.width();
    int eheight = _explicitPreferredSize.height();
    if (ewidth != -1 && eheight != -1) {
        return _explicitPreferredSize;
    }

    // if not, we must compute; first get the border margins, if any
    int mwidth = 0, mheight = 0;
    if (_border != 0) {
        QMargins margins = _border->margins();
        mwidth = margins.left() + margins.right();
        mheight = margins.top() + margins.bottom();
    }

    // subtract them from the hints, if provided
    if (whint != -1) {
        whint -= mwidth;
    }
    if (hhint != -1) {
        hhint -= mheight;
    }

    // compute the preferred size
    QSize computed = computePreferredSize(whint, hhint);

    // add back the margins
    return QSize(
        (ewidth == -1) ? (computed.width() + mwidth) : eheight,
        (eheight == -1) ? (computed.height() + mheight) : eheight);
}

void Component::setConstraint (const QVariant& constraint)
{
    if (_constraint != constraint) {
        _constraint = constraint;
        invalidateParent();
    }
}

void Component::maybeValidate ()
{
    if (!_valid) {
        validate();
        _valid = true;
    }
}

void Component::invalidate ()
{
    if (_valid) {
        _valid = false;

        // invalidate all the way up the line
        invalidateParent();
    }
}

void Component::validate ()
{
}

QSize Component::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, 0), qMax(hhint, 0));
}

void Component::invalidateParent () const
{
    Component* pcomp = qobject_cast<Component*>(parent());
    if (pcomp != 0) {
        pcomp->invalidate();
    }
}

Container::Container (QObject* parent) :
    Component(parent),
    _layout(0)
{
}

Container::~Container ()
{
    if (_layout != 0) {
        delete _layout;
    }
}

void Container::setLayout (Layout* layout)
{
    if (_layout != layout) {
        if (_layout != 0) {
            delete _layout;
        }
        _layout = layout;
        invalidate();
    }
}

void Container::addChild (Component* child, const QVariant& constraint)
{
    _children.append(child);
    child->setParent(this);
    child->setConstraint(constraint);
    invalidate();
}

void Container::removeChild (Component* child)
{
    if (_children.removeOne(child)) {
        child->setParent(0);
        invalidate();
    }
}

void Container::validate ()
{
    // apply the layout, if any
    if (_layout != 0) {
        _layout->apply(this);
    }

    // validate all children
    foreach (Component* child, _children) {
        child->maybeValidate();
    }
}

QSize Container::computePreferredSize (int whint, int hhint) const
{
    // if we have a layout, that computes the size
    if (_layout != 0) {
        return _layout->computePreferredSize(this, whint, hhint);
    }

    // otherwise, use the bounds of the children
    int mwidth = qMax(whint, 0);
    int mheight = qMax(hhint, 0);
    foreach (Component* child, _children) {
        QRect bounds = child->bounds();
        mwidth = qMax(bounds.x() + bounds.width(), mwidth);
        mheight = qMax(bounds.y() + bounds.height(), mheight);
    }
    return QSize(mwidth, mheight);
}
