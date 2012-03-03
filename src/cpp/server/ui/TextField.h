//
// $Id$

#ifndef TEXT_FIELD
#define TEXT_FIELD

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
    Document (const QString& text = "");

    /**
     * Returns the text of the document.
     */
    const QString& text () const { return _text; }

    /**
     * Inserts a string into the document.
     */
    virtual void insert (int idx, const QString& text);

    /**
     * Deletes a string from the document.
     */
    void remove (int idx, int length);

protected:

    /** The text of the document. */
    QString _text;
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
    TextField (int minWidth = 20, Document* document = new Document(), QObject* parent = 0);

    /**
     * Creates a new text field.
     */
    TextField (int minWidth, const QString& text, QObject* parent = 0);

    /**
     * Destroys the text field.
     */
    virtual ~TextField ();

    /**
     * Returns the contents of the field.
     */
    const QString& text () const { return _document->text(); }

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return true; }

signals:

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
    virtual void draw (DrawContext* ctx) const;

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
     */
    void updateDocumentPos ();

    /** The minimum width. */
    int _minWidth;

    /** The document containing the contents. */
    Document* _document;

    /** The position in the document. */
    int _documentPos;

    /** The cursor position. */
    int _cursorPos;
};

#endif // TEXT_FIELD
