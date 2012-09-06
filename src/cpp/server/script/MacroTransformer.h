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
     * Determines whether the provided form matches this pattern.
     */
    virtual bool matches (ScriptObjectPointer form) = 0;
};

/**
 * A pattern that matches a constant form.
 */
class ConstantPattern : public Pattern
{
public:

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

protected:

    /** The index of the variable to store under, or -1 for none. */
    int _index;
};

/**
 * A pattern that matches a literal identifier with the same lexical binding.
 */
class LiteralPattern : public Pattern
{
public:

};

/**
 * A pattern that matches a list with various subpatterns.
 */
class ListPattern : public Pattern
{
public:

protected:

    /** The first patterns (before the repeating pattern). */
    QVector<PatternPointer> _preRepeatPatterns;
    
    /** An optional repeating pattern. */
    PatternPointer _repeatPattern;
    
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
};

/**
 * A template that emits the contents of a pattern variable.
 */
class VariableTemplate : public Template
{
public:

protected:
    
    /** The index of the variable to emit. */
    int _index;
};

/**
 * A template that emits a fixed datum.
 */
class DatumTemplate : public Template
{
public:

protected:
    
    /** The datum to emit. */
    ScriptObjectPointer _datum;
};

/**
 * A template that emits a list with various subtemplates.
 */
class ListTemplate : public Template
{
public:

protected:
    
    /** Pairs a template with the number of ellipses following it. */
    typedef QPair<TemplatePointer, int> Subtemplate;
    
    /** The list of subtemplates. */
    QVector<Subtemplate> _subtemplates;
    
    /** An optional template for the rest of the list. */
    TemplatePointer _restTemplate;
};

#endif // MACRO_TRANSFORMER
