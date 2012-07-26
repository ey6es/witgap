//
// $Id$

/**
 * Constructor for Point objects, after flash.geom.Point.
 */
function Point (x, y)
{
    this.x = x;
    this.y = y;
}

/**
 * Checks whether this point equals another.
 */
Point.prototype.equals = function (opoint)
{
    return this.x == opoint.x && this.y == opoint.y;
};

/**
 * Constructor for Rectangle objects, after flash.geom.Rectangle.
 */
function Rectangle (x, y, width, height)
{
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
}

/**
 * Returns the intersection of this and the specified rectangle as a new Rectangle.
 */
Rectangle.prototype.intersection = function (orect)
{
    if (this.isEmpty()) {
        return orect.clone();
    }
    if (orect.isEmpty()) {
        return this.clone();
    }
    var mx = Math.max(this.x, orect.x);
    var my = Math.max(this.y, orect.y);
    return new Rectangle(mx, my,
        Math.min(this.x + this.width, orect.x + orect.width) - mx,
        Math.min(this.y + this.height, orect.y + orect.height) - my);
};

/**
 * Returns the union of this and the specified rectangle as a new Rectangle.
 */
Rectangle.prototype.union = function (orect)
{
    if (this.isEmpty()) {
        return orect.clone();
    }
    if (orect.isEmpty()) {
        return this.clone();
    }
    var mx = Math.min(this.x, orect.x);
    var my = Math.min(this.y, orect.y);
    return new Rectangle(mx, my,
        Math.max(this.x + this.width, orect.x + orect.width) - mx,
        Math.max(this.y + this.height, orect.y + orect.height) - my);
};

/**
 * Sets this to an empty rectangle.
 */
Rectangle.prototype.setEmpty = function ()
{
    this.x = this.y = this.width = this.height = 0;
};

/**
 * Determines whether the rectangle is empty (whether its width or height is <= 0).
 */
Rectangle.prototype.isEmpty = function ()
{
    return this.width <= 0 || this.height <= 0;
};

/**
 * Returns the size of the rectangle as a Point.
 */
Rectangle.prototype.size = function ()
{
    return new Point(this.width, this.height);
};

/**
 * Returns a cloned copy of this rectangle.
 */
Rectangle.prototype.clone = function ()
{
    return new Rectangle(this.x, this.y, this.width, this.height);
};

/**
 * Checks whether this rectangle equals another.
 */
Rectangle.prototype.equals = function (orect)
{
    return this.x == orect.x && this.y == orect.y &&
        this.width == orect.width && this.height == orect.height;
};

/**
 * Sets the fields of the rectangle.
 */
Rectangle.prototype.setTo = function (x, y, width, height)
{
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
};

/**
 * Copies values from another rectangle.
 */
Rectangle.prototype.copyFrom = function (orect)
{
    this.x = orect.x;
    this.y = orect.y;
    this.width = orect.width;
    this.height = orect.height;
};
