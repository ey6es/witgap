//
// $Id$

#ifndef TEXT_FIELD
#define TEXT_FIELD

#include <QPair>
#include <QRegExp>
#include <QStringList>
#include <QTimer>
#include <QUndoStack>

#include "ui/Component.h"

class TextEditCommand;

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
 * Base class for text editing components.
 */
class TextComponent : public Component
{
    Q_OBJECT

    friend class TextEditCommand;
    
public:
    
    /**
     * Creates a new text component.
     */
    TextComponent (int minWidth = 20, Document* document = new Document(), QObject* parent = 0);
    
    /**
     * Destroys the text component.
     */
    virtual ~TextComponent ();
    
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
    virtual void setDocument (Document* document) = 0;
    
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
     * Enables or disables parenthesis matching.
     */
    void setMatchParentheses (bool match);

    /**
     * Checks whether parenthesis matching is enabled.
     */
    bool matchParentheses () const { return _matchParentheses; }
    
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
     * Emitted when the enter key is pressed.
     */
    void enterPressed ();
    
protected slots:

    /**
     * Clears the currently displayed match.
     */
    virtual void clearMatch () = 0;
    
protected:
    
    /**
     * Validates the component.
     */
    virtual void validate ();
    
    /**
     * Draws part of the text.
     */
    virtual void drawText (DrawContext* ctx, int x, int y, int idx, int length, int flags) const;
    
    /**
     * Returns the character to display for the specified index.
     */
    virtual int visibleChar (int idx) const;
    
    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);
    
    /**
     * Inserts text into the document.
     *
     * @param cursorAfter whether to place the cursor after the inserted text (as opposed to at
     * its beginning).
     * @return the text that was actually inserted (i.e., after filtering).
     */
    virtual QString insert (int idx, const QString& text, bool cursorAfter) = 0;
    
    /**
     * Removes text from the document.
     */
    virtual void remove (int idx, int length) = 0;
    
    /**
     * Depending on the cursor location and the character under the cursor, considers showing the
     * matching parenthesis.
     */
    void maybeShowMatch ();
    
    /**
     * Considers showing the matched parenthesis for the character at the specified location.
     *
     * @return whether or not the location contained a parenthesis to match.
     */
    bool maybeShowMatch (int idx);
    
    /**
     * Shows a match at the specified index.
     */
    virtual void showMatch (int idx) = 0;
    
    /**
     * Updates the document position to match the cursor position.
     *
     * @return whether or not the component was dirtied as a result of the update.
     */
    virtual bool updateDocumentPos () = 0;

    /**
     * Dirties the region corresponding to the described document section.
     */
    virtual void dirty (int idx, int length) = 0;
    
    /** The minimum width. */
    int _minWidth;

    /** The document containing the contents. */
    Document* _document;
    
    /** The position in the document. */
    int _documentPos;

    /** The cursor position. */
    int _cursorPos;

    /** If not empty, a string to display when the document is blank and the field lacks focus. */
    QString _label;

    /** Whether or not parenthesis matching is enabled. */
    bool _matchParentheses;

    /** The position of the displayed match, or -1 for none. */
    int _matchPos;

    /** The match timer. */
    QTimer _matchTimer;

    /** The undo stack. */
    QUndoStack _undoStack;
};

/**
 * A text field.
 */
class TextField : public TextComponent
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
     * Sets the document.
     */
    virtual void setDocument (Document* document);
    
signals:

    /**
     * Emitted when the text is full (at maximum length).
     */
    void textFull ();

protected:
    
    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx);

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
     * Inserts text into the document.
     *
     * @param cursorAfter whether to place the cursor after the inserted text (as opposed to at
     * its beginning).
     * @return the text that was actually inserted (i.e., after filtering).
     */
    virtual QString insert (int idx, const QString& text, bool cursorAfter);
    
    /**
     * Removes text from the document.
     */
    virtual void remove (int idx, int length);
    
    /**
     * Shows a match at the specified index.
     */
    virtual void showMatch (int idx);
    
    /**
     * Clears the currently displayed match.
     */
    virtual void clearMatch ();
    
    /**
     * Updates the document position to match the cursor position.
     *
     * @return whether or not the component was dirtied as a result of the update.
     */
    virtual bool updateDocumentPos ();

    /**
     * Dirties the region starting at the supplied document index and including everything after.
     *
     * @param deleted the number of characters deleted from the document, if any.
     */
    void dirtyRest (int idx, int deleted);

    /**
     * Dirties the region corresponding to the described document section.
     */
    virtual void dirty (int idx, int length);

    /**
     * Returns the width of the actual text area.
     */
    int textAreaWidth () const;

    /** Whether or not to right-align the contents. */
    bool _rightAlign;
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
     * Returns the character to display for the specified index.
     */
    virtual int visibleChar (int idx) const;
};

/**
 * Describes a span within a document.
 */
class Span
{
public:
    
    /**
     * Creates a new span.
     */
    Span (int start = 0, int length = 0);
    
    /** The start of the span. */
    int start;
    
    /** The length of the span. */
    int length;
};

/**
 * Allows editing multiple lines of text.
 */
class TextArea : public TextComponent
{
    Q_OBJECT

public:
    
    /**
     * Creates a new text area.
     */
    TextArea (int minWidth = 20, int minHeight = 3,  Document* document = new Document("", 1024),
        bool wordWrap = true, QObject* parent = 0);
    
    /**
     * Creates a new text area.
     */
    TextArea (int minWidth, int minHeight, const QString& text,
        bool wordWrap = true, QObject* parent = 0);

    /**
     * Enables or disables word wrap.
     */
    void setWordWrap (bool wrap);

    /**
     * Checks whether word wrap is enabled.
     */
    bool wordWrap () const { return _wordWrap; }

    /**
     * Sets the document.
     */
    virtual void setDocument (Document* document);
    
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
     * Clears the currently displayed match.
     */
    virtual void clearMatch ();
    
    /**
     * Inserts text into the document.
     *
     * @param cursorAfter whether to place the cursor after the inserted text (as opposed to at
     * its beginning).
     * @return the text that was actually inserted (i.e., after filtering).
     */
    virtual QString insert (int idx, const QString& text, bool cursorAfter);
    
    /**
     * Removes text from the document.
     */
    virtual void remove (int idx, int length);
    
    /**
     * Shows a match at the specified index.
     */
    virtual void showMatch (int idx);
    
    /**
     * Updates the document position to match the cursor position.
     *
     * @return whether or not the component was dirtied as a result of the update.
     */
    virtual bool updateDocumentPos ();

    /**
     * Dirties the region corresponding to the described document section.
     */
    virtual void dirty (int idx, int length);
    
    /** The minimum height of the component. */
    int _minHeight;
    
    /** Whether or not to wrap at word boundaries. */
    bool _wordWrap;
    
    /** The line spans. */
    QVector<Span> _lines;
    
    /** The index of the line containing the cursor. */
    int _cursorLine;
};

/** An expression that simply requires the text to contain something other than whitespace. */
const QRegExp NonEmptyExp("\\s*\\S+.*");

/**
 * Enables a component based on whether the contents of one or more text components match regular
 * expressions.
 */
class FieldExpEnabler : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new enabler.
     */
    FieldExpEnabler (Component* component, TextComponent* f1, const QRegExp& e1 = NonEmptyExp,
        TextComponent* f2 = 0, const QRegExp& e2 = NonEmptyExp,
        TextComponent* f3 = 0, const QRegExp& e3 = NonEmptyExp,
        TextComponent* f4 = 0, const QRegExp& e4 = NonEmptyExp);

public slots:

    /**
     * Updates the component based on the text field contents.
     */
    void updateComponent ();

protected:

    /** A text component and its corresponding expression. */
    typedef QPair<TextComponent*, QRegExp> FieldExp;

    /** The component to enable. */
    Component* _component;

    /** The field/regexp pairs. */
    QVector<FieldExp> _fieldExps;
};

/**
 * An undo command representing a text edit (insertion or deletion).
 */
class TextEditCommand : public QUndoCommand
{
public:

    /**
     * Creates a new insert command.
     */
    TextEditCommand (TextComponent* component, int idx, const QString& text);

    /**
     * Creates a new delete command.
     *
     * @param backspace whether the deletion was a result of backspacing.
     */
    TextEditCommand (TextComponent* component, int idx, int length, bool backspace);

    /**
     * Undo the edit.
     */
    virtual void undo ();

    /**
     * Redo the edit.
     */
    virtual void redo ();

    /**
     * Returns the merge id.
     */
    virtual int id () const;

    /**
     * Attempts to merge this edit with another.
     */
    virtual bool mergeWith (const QUndoCommand* command);

protected:

    /**
     * Performs or reverses the edit.
     */
    void apply (bool reverse);

    /** The text component upon which the edit was performed. */
    TextComponent* _component;

    /** If true, the edit was an insertion; otherwise, a deletion. */
    bool _insertion;

    /** If true and the edit was a deletion, whether it was the result of backspacing. */
    bool _backspace;

    /** The index of the edit. */
    int _idx;

    /** The changed text. */
    QString _text;
};

#endif // TEXT_FIELD
