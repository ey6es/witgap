//
// $Id$

#ifndef TEXT_FIELD
#define TEXT_FIELD

#include <QPair>
#include <QRegExp>

#include "ui/Component.h"

/**
 * Contains and controls the contents of a text field.
 */
class Document
{
public:

    /**
     * Creates a new document with the supplied initial text.
     */
    Document (const QString& text = "", int maxLength = 255);

    /**
     * Returns the text of the document.
     */
    const QString& text () const { return _text; }

    /**
     * Attempts to insert a string into the document.
     */
    virtual void insert (int idx, const QString& text);

    /**
     * Deletes a string from the document.
     */
    void remove (int idx, int length);

    /**
     * Checks whether the document has reached its maximum length.
     */
    bool full () const { return _text.length() == _maxLength; }

protected:

    /** The text of the document. */
    QString _text;

    /** The maximum allowed length. */
    int _maxLength;
};

/**
 * A document restricted to strings that match a regular expression.
 */
class RegExpDocument : public Document
{
public:

    /**
     * Creates a new document with the supplied initial text.
     */
    RegExpDocument (const QRegExp& regExp, const QString& text = "", int maxLength = 255);

    /**
     * Attempts to insert a string into the document.
     */
    virtual void insert (int idx, const QString& text);

protected:

    /** The regular expression that determines what to allow. */
    QRegExp _regExp;
};

/**
 * A text field.
 */
class TextField : public Component
{
    Q_OBJECT

public:

    /**
     * Creates a new text field.
     */
    TextField (int minWidth = 20, Document* document = new Document(),
        bool rightAlign = false, QObject* parent = 0);

    /**
     * Creates a new text field.
     */
    TextField (int minWidth, const QString& text,
        bool rightAlign = false, QObject* parent = 0);

    /**
     * Destroys the text field.
     */
    virtual ~TextField ();

    /**
     * Sets the text within the document.
     */
    void setText (const QString& text);

    /**
     * Returns the contents of the field.
     */
    const QString& text () const { return _document->text(); }

    /**
     * Sets the document.
     */
    void setDocument (Document* document);

    /**
     * Sets the position of the cursor within the document.
     */
    void setCursorPosition (int pos);

    /**
     * Returns the cursor position.
     */
    int cursorPosition () const { return _cursorPos; }

    /**
     * Sets the label to display when the document is empty and the field lacks focus.
     */
    void setLabel (const QString& label);

    /**
     * Returns a reference to the label.
     */
    const QString& label () const { return _label; }

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return _enabled && _visible; }

signals:

    /**
     * Emitted when the text has changed.
     */
    void textChanged ();

    /**
     * Emitted when the text is full (at maximum length).
     */
    void textFull ();

    /**
     * Emitted when the enter key is pressed.
     */
    void enterPressed ();

protected:

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Validates the component.
     */
    virtual void validate ();

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx);

    /**
     * Draws part of the text.
     */
    virtual void drawText (DrawContext* ctx, int x, int y, int idx, int length, int flags) const;

    /**
     * Returns the character to display under the cursor.
     */
    virtual int cursorChar () const;

    /**
     * Handles a focus in event.
     */
    virtual void focusInEvent (QFocusEvent* e);

    /**
     * Handles a focus out event.
     */
    virtual void focusOutEvent (QFocusEvent* e);

    /**
     * Handles a mouse press event.
     */
    virtual void mouseButtonPressEvent (QMouseEvent* e);

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Updates the document position to match the cursor position.
     *
     * @return whether or not the component was dirtied as a result of the update.
     */
    bool updateDocumentPos ();

    /**
     * Dirties the region starting at the supplied document index and including everything after.
     */
    void dirty (int idx);

    /**
     * Dirties the region corresponding to the described document section.
     */
    void dirty (int idx, int length);

    /**
     * Returns the width of the actual text area.
     */
    int textAreaWidth () const;

    /** The minimum width. */
    int _minWidth;

    /** The document containing the contents. */
    Document* _document;

    /** Whether or not to right-align the contents. */
    bool _rightAlign;

    /** The position in the document. */
    int _documentPos;

    /** The cursor position. */
    int _cursorPos;

    /** If not empty, a string to display when the document is blank and the field lacks focus. */
    QString _label;
};

/**
 * A text field that displays its text as asterisks.
 */
class PasswordField : public TextField
{
    Q_OBJECT

public:

    /**
     * Creates a new password field.
     */
    PasswordField (int minWidth = 20, Document* document = new Document(), QObject* parent = 0);

    /**
     * Creates a new password field.
     */
    PasswordField (int minWidth, const QString& text, QObject* parent = 0);

protected:

    /**
     * Draws part of the text.
     */
    virtual void drawText (DrawContext* ctx, int x, int y, int idx, int length, int flags) const;

    /**
     * Returns the character to display under the cursor.
     */
    virtual int cursorChar () const;
};

/** An expression that simply requires the text to contain something other than whitespace. */
const QRegExp NonEmptyExp("\\s*\\S+.*");

/**
 * Enables a component based on whether the contents of one or more text fields match regular
 * expressions.
 */
class FieldExpEnabler : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new enabler.
     */
    FieldExpEnabler (Component* component, TextField* f1, const QRegExp& e1 = NonEmptyExp,
        TextField* f2 = 0, const QRegExp& e2 = NonEmptyExp,
        TextField* f3 = 0, const QRegExp& e3 = NonEmptyExp,
        TextField* f4 = 0, const QRegExp& e4 = NonEmptyExp);

public slots:

    /**
     * Updates the component based on the text field contents.
     */
    void updateComponent ();

protected:

    /** A text field and its corresponding expression. */
    typedef QPair<TextField*, QRegExp> FieldExp;

    /** The component to enable. */
    Component* _component;

    /** The field/regexp pairs. */
    QVector<FieldExp> _fieldExps;
};

#endif // TEXT_FIELD
