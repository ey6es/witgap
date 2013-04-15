//
// $Id$

#ifndef MACRO_TRANSFORMER
#define MACRO_TRANSFORMER

#include "script/Evaluator.h"

/**
 * Creates a macro transformer from the supplied expression.  Throws ScriptError if there is one.
 */
ScriptObjectPointer createMacroTransformer (ScriptObjectPointer expr, Scope* scope);

/**
 * Base class of patterns for matching forms.
 */
class Pattern
{
public:

    /**
     * Determines whether the provided form matches this pattern.  If so, populates the supplied
     * variable list.
     */
    virtual bool matches (
        ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables) const = 0;
};

/**
 * A pattern that matches a constant form.
 */
class ConstantPattern : public Pattern
{
public:

    /**
     * Creates a new constant pattern.
     */
    ConstantPattern (const ScriptObjectPointer& constant);

    /**
     * Determines whether the provided form matches this pattern.  If so, populates the supplied
     * variable list.
     */
    virtual bool matches (
        ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The constant to compare to. */
    ScriptObjectPointer _constant;
};

/**
 * A pattern that matches any form and optionally stores it under a variable.
 */
class VariablePattern : public Pattern
{
public:

    /**
     * Creates a new variable pattern.
     */
    VariablePattern (int index);

    /**
     * Determines whether the provided form matches this pattern.  If so, populates the supplied
     * variable list.
     */
    virtual bool matches (
        ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The index of the variable to store under, or -1 for none. */
    int _index;
};

/**
 * A pattern that matches a literal identifier with the same lexical binding.
 */
class BoundLiteralPattern : public Pattern
{
public:

    /**
     * Creates a new bound literal pattern.
     */
    BoundLiteralPattern (const ScriptObjectPointer& binding);

    /**
     * Determines whether the provided form matches this pattern.  If so, populates the supplied
     * variable list.
     */
    virtual bool matches (
        ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The binding of the identifier. */
    ScriptObjectPointer _binding;
};

/**
 * A pattern that matches a literal identifier with the same name and no lexical binding.
 */
class UnboundLiteralPattern : public Pattern
{
public:

    /**
     * Creates a new unbound literal pattern.
     */
    UnboundLiteralPattern (const QString& name);

    /**
     * Determines whether the provided form matches this pattern.  If so, populates the supplied
     * variable list.
     */
    virtual bool matches (
        ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The name of the identifier. */
    QString _name;
};

/**
 * A pattern that matches a vector with various subpatterns.
 */
class VectorPattern : public Pattern
{
public:

    /**
     * Creates a new vector pattern.
     */
    VectorPattern (const QVector<PatternPointer>& preRepeatPatterns,
        const PatternPointer& repeatPattern, int repeatVariableIdx, int repeatVariableCount,
        const QVector<PatternPointer>& postRepeatPatterns, const PatternPointer& restPattern);

    /**
     * Determines whether the provided form matches this pattern.  If so, populates the supplied
     * variable list.
     */
    virtual bool matches (
        ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The first patterns (before the repeating pattern). */
    QVector<PatternPointer> _preRepeatPatterns;

    /** An optional repeating pattern. */
    PatternPointer _repeatPattern;

    /** The index of the first variable in the repeating pattern. */
    int _repeatVariableIdx;

    /** The number of variables in the repeating pattern. */
    int _repeatVariableCount;

    /** The second patterns (after the repeating pattern). */
    QVector<PatternPointer> _postRepeatPatterns;

    /** An optional list pattern for the rest. */
    PatternPointer _restPattern;
};

/**
 * Base class for templates that generate forms from pattern variables.
 */
class Template
{
public:

    /**
     * Generates the contents of the template.
     */
    virtual ScriptObjectPointer generate (QVector<ScriptObjectPointer>& variables) const = 0;
};

/**
 * A template that emits a fixed datum.
 */
class DatumTemplate : public Template
{
public:

    /**
     * Creates a new datum template.
     */
    DatumTemplate (const ScriptObjectPointer& datum);

    /**
     * Generates the contents of the template.
     */
    virtual ScriptObjectPointer generate (QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The datum to emit. */
    ScriptObjectPointer _datum;
};

/**
 * A template that emits the binding of a symbol in the template scope.
 */
class SymbolTemplate : public Template
{
public:

    /**
     * Creates a new symbol template.
     */
    SymbolTemplate (Scope* scope, const ScriptObjectPointer& datum);

    /**
     * Generates the contents of the template.
     */
    virtual ScriptObjectPointer generate (QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The template scope pointer. */
    Scope* _scope;

    /** The symbol datum. */
    ScriptObjectPointer _datum;
};

/**
 * A template that emits the contents of a pattern variable.
 */
class VariableTemplate : public Template
{
public:

    /**
     * Creates a new variable template.
     */
    VariableTemplate (int index);

    /**
     * Generates the contents of the template.
     */
    virtual ScriptObjectPointer generate (QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The index of the variable to emit. */
    int _index;
};

/**
 * A subtemplate within a list template.
 */
class Subtemplate
{
public:

    /**
     * Creates a new subtemplate.
     */
    Subtemplate (int repeatVariableIdx, int repeatVariableCount,
        int repeatDepth, const TemplatePointer& templ);

    /**
     * Creates an invalid subtemplate.
     */
    Subtemplate ();

    /**
     * Generates the contents of the subtemplate.
     */
    void generate (QVector<ScriptObjectPointer>& variables, ScriptObjectPointerVector& out) const;

protected:

    /**
     * Generates the contents of the template.
     */
    void generate (QVector<ScriptObjectPointer>& variables,
        ScriptObjectPointerVector& out, int depth) const;

    /** The start of the variable range to unwrap. */
    int _repeatVariableIdx;

    /** The length of the variable range to unwrap. */
    int _repeatVariableCount;

    /** The repeat depth. */
    int _repeatDepth;

    /** The template to generate. */
    TemplatePointer _template;
};

/**
 * A template that emits a list with various subtemplates.
 */
class ListTemplate : public Template
{
public:

    /**
     * Creates a new list template.
     */
    ListTemplate (const QVector<Subtemplate>& subtemplates, const TemplatePointer& restTemplate);

    /**
     * Generates the contents of the template.
     */
    virtual ScriptObjectPointer generate (QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The list of subtemplates. */
    QVector<Subtemplate> _subtemplates;

    /** An optional template for the rest of the list. */
    TemplatePointer _restTemplate;
};

/**
 * A template that emits a vector with various subtemplates.
 */
class VectorTemplate : public Template
{
public:

    /**
     * Creates a new vector template.
     */
    VectorTemplate (const QVector<Subtemplate>& subtemplates, const TemplatePointer& restTemplate);

    /**
     * Generates the contents of the template.
     */
    virtual ScriptObjectPointer generate (QVector<ScriptObjectPointer>& variables) const;

protected:

    /** The list of subtemplates. */
    QVector<Subtemplate> _subtemplates;

    /** An optional template for the rest of the list. */
    TemplatePointer _restTemplate;
};

#endif // MACRO_TRANSFORMER
