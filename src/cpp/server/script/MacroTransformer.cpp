//
// $Id$

#include "script/MacroTransformer.h"

/**
 * Creates a pattern from the specified expression.
 */
static PatternPointer createPattern (
    ScriptObjectPointer expr, const QHash<QString, PatternPointer>& literals,
    QHash<QString, int>& variables)
{
    switch (expr->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            const QString& name = symbol->name();
            PatternPointer lpat = literals.value(name);
            if (!lpat.isNull()) {
                return lpat;
            }
            if (name == "_") {
                return PatternPointer(new VariablePattern(-1));
            }
            if (variables.contains(name)) {
                throw ScriptError("Duplicate variable.", symbol->position());
            }
            int index = variables.size();
            variables.insert(name, index);
            return PatternPointer(new VariablePattern(index));
        }
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const QList<ScriptObjectPointer>& contents = list->contents();
            QVector<PatternPointer> preRepeatPatterns;
            PatternPointer repeatPattern;
            int repeatVariableIdx;
            int repeatVariableCount;
            QVector<PatternPointer> postRepeatPatterns;
            PatternPointer restPattern;
            for (int ii = 0, nn = contents.size(); ii < nn; ii++) {
                if (ii + 1 < nn) {
                    ScriptObjectPointer next = contents.at(ii + 1);
                    if (next->type() == ScriptObject::SymbolType) {
                        Symbol* symbol = static_cast<Symbol*>(next.data());
                        if (symbol->name() == "...") {
                            repeatVariableIdx = variables.size();
                            repeatPattern = createPattern(contents.at(ii), literals, variables);
                            repeatVariableCount = variables.size() - repeatVariableIdx;
                            for (ii += 2; ii < nn; ii++) {
                                ScriptObjectPointer element = contents.at(ii);
                                if (element->type() == ScriptObject::SymbolType) {
                                    symbol = static_cast<Symbol*>(element.data());
                                    if (symbol->name() == ".") {
                                        if (nn != ii + 2) {
                                            throw ScriptError("Invalid pattern.",
                                                list->position());
                                        }
                                        restPattern = createPattern(
                                            contents.at(ii + 1), literals, variables);
                                        break;
                                    }
                                }
                                postRepeatPatterns.append(createPattern(
                                    contents.at(ii), literals, variables));
                            }
                            break;
                        }
                    }
                }
                preRepeatPatterns.append(createPattern(contents.at(ii), literals, variables));
            }
            return PatternPointer(new ListPattern(
                preRepeatPatterns, repeatPattern, repeatVariableIdx,
                repeatVariableCount, postRepeatPatterns, restPattern));
        }
        default:
            return PatternPointer(new ConstantPattern(expr));
    }
}

/**
 * Creates a template from the specified expression.
 *
 * @ellipseIdents if true, treat ellipses as normal identifiers.
 */
static TemplatePointer createTemplate (
    ScriptObjectPointer expr, const QHash<QString, int>& variables, bool ellipseIdents = false)
{
    switch (expr->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            const QString& name = symbol->name();
            QHash<QString, int>::const_iterator it = variables.constFind(name);
            if (it == variables.constEnd()) {
                return TemplatePointer(new DatumTemplate(expr));
            }
            return TemplatePointer(new VariableTemplate(*it));
        }
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const QList<ScriptObjectPointer>& contents = list->contents();
            QVector<Subtemplate> subtemplates;
            TemplatePointer restTemplate;
            for (int ii = 0, nn = contents.size(); ii < nn; ii++) {
                ScriptObjectPointer element = contents.at(ii);
                if (element->type() == ScriptObject::SymbolType) {
                    Symbol* symbol = static_cast<Symbol*>(element.data());
                    const QString& name = symbol->name();
                    if (name == ".") {
                        if (nn != ii + 2) {
                            throw ScriptError("Invalid template.", list->position());
                        }
                        restTemplate = createTemplate(contents.at(ii + 1),
                            variables, ellipseIdents);
                        break;
                    }
                    if (name == "..." && !ellipseIdents) {
                        if (ii == 0) {
                            if (nn != 2) {
                                throw ScriptError("Invalid template.", list->position());
                            }
                            return createTemplate(contents.at(1), variables, true);
                        }
                        
                    }
                }
            }
            return TemplatePointer(new ListTemplate(subtemplates, restTemplate));
        }
        default:
            return TemplatePointer(new DatumTemplate(expr));
    }
}

ScriptObjectPointer createMacroTransformer (ScriptObjectPointer expr, Scope* scope)
{
    Datum* datum = static_cast<Datum*>(expr.data());
    switch (expr->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            ScriptObjectPointer binding = scope->resolve(symbol->name());
            if (binding.isNull()) {
                throw ScriptError("Unresolved symbol.", symbol->position());
            }
            switch (binding->type()) {
                case ScriptObject::SyntaxRulesType:
                case ScriptObject::IdentifierSyntaxType:
                    return binding;
                default:
                    throw ScriptError("Not a macro transformer.", symbol->position());
            }
        }
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const QList<ScriptObjectPointer>& contents = list->contents();
            int csize = contents.size();
            if (csize < 2) {
                throw ScriptError("Invalid macro transformer.", list->position());
            }
            ScriptObjectPointer car = contents.at(0);
            if (car->type() != ScriptObject::SymbolType) {
                throw ScriptError("Invalid macro transformer.", list->position());
            }
            Symbol* symbol = static_cast<Symbol*>(car.data());
            const QString& name = symbol->name();
            if (name == "syntax-rules") {
                ScriptObjectPointer cadr = contents.at(1);
                if (cadr->type() != ScriptObject::ListType) {
                    throw ScriptError("Invalid syntax rules.", list->position());
                }
                List* llist = static_cast<List*>(cadr.data());
                QHash<QString, PatternPointer> literals;
                foreach (const ScriptObjectPointer& element, llist->contents()) {
                    if (element->type() != ScriptObject::SymbolType) {
                        throw ScriptError("Invalid literals.", llist->position());
                    }
                    Symbol* literal = static_cast<Symbol*>(element.data());
                    const QString& lname = literal->name();
                    ScriptObjectPointer binding = scope->resolve(lname);
                    literals.insert(lname, PatternPointer(binding.isNull() ?
                        (Pattern*)new UnboundLiteralPattern(lname) :
                        (Pattern*)new BoundLiteralPattern(binding)));
                }
                QVector<PatternTemplate> patternTemplates;
                for (int ii = 2; ii < csize; ii++) {
                    ScriptObjectPointer rule = contents.at(ii);
                    if (rule->type() != ScriptObject::ListType) {
                        throw ScriptError("Invalid rule.",
                            static_cast<Datum*>(rule.data())->position());
                    }
                    List* rlist = static_cast<List*>(rule.data());
                    const QList<ScriptObjectPointer>& rcontents = rlist->contents();
                    if (rcontents.size() != 2) {
                        throw ScriptError("Invalid rule.", rlist->position());
                    }
                    QHash<QString, int> variables;
                    PatternPointer pattern = createPattern(rcontents.at(0), literals, variables);
                    TemplatePointer templ = createTemplate(rcontents.at(1), variables);
                    patternTemplates.append(PatternTemplate(variables.size(), pattern, templ));
                }
                return ScriptObjectPointer(new SyntaxRules(patternTemplates));
                
            } else if (name == "identifier-syntax") {
//                return ScriptObjectPointer(new IdentifierSyntax());
                
            } else {
                throw ScriptError("Invalid macro transformer.", list->position());
            }
        }
        default:
            throw ScriptError("Not a macro transformer.", datum->position());
    }
}

ConstantPattern::ConstantPattern (const ScriptObjectPointer& constant) :
    _constant(constant)
{
}

bool ConstantPattern::matches (
    ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables)
{
    return equivalent(form, _constant);
}

VariablePattern::VariablePattern (int index) :
    _index(index)
{
}

bool VariablePattern::matches (
    ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables)
{
    if (_index != -1) {
        variables[_index] = form;
    }
    return true;
}

BoundLiteralPattern::BoundLiteralPattern (const ScriptObjectPointer& binding) :
    _binding(binding)
{
}

bool BoundLiteralPattern::matches (
    ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables)
{
    if (form->type() != ScriptObject::SymbolType) {
        return false;
    }
    Symbol* symbol = static_cast<Symbol*>(form.data());
    ScriptObjectPointer obinding = scope->resolve(symbol->name());
    return !obinding.isNull() && equivalent(obinding, _binding);
}

UnboundLiteralPattern::UnboundLiteralPattern (const QString& name) :
    _name(name)
{
}

bool UnboundLiteralPattern::matches (
    ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables)
{
    if (form->type() != ScriptObject::SymbolType) {
        return false;
    }
    Symbol* symbol = static_cast<Symbol*>(form.data());
    return symbol->name() == _name && scope->resolve(_name).isNull();
}

ListPattern::ListPattern (const QVector<PatternPointer>& preRepeatPatterns,
        const PatternPointer& repeatPattern, int repeatVariableIdx, int repeatVariableCount,
        const QVector<PatternPointer>& postRepeatPatterns, const PatternPointer& restPattern) :
    _preRepeatPatterns(preRepeatPatterns),
    _repeatPattern(repeatPattern),
    _repeatVariableIdx(repeatVariableIdx),
    _repeatVariableCount(repeatVariableCount),
    _postRepeatPatterns(postRepeatPatterns),
    _restPattern(restPattern)
{
}

bool ListPattern::matches (
    ScriptObjectPointer form, Scope* scope, QVector<ScriptObjectPointer>& variables)
{
    if (form->type() != ScriptObject::ListType) {
        return false;
    }
    List* list = static_cast<List*>(form.data());
    const QList<ScriptObjectPointer>& contents = list->contents();
    int size = contents.size();
    int precount = _preRepeatPatterns.size();
    int postcount = _postRepeatPatterns.size();
    if (size < precount + postcount) {
        return false;
    }
    int idx = 0;
    for (; idx < precount; idx++) {
        if (!_preRepeatPatterns.at(idx)->matches(contents.at(idx), scope, variables)) {
            return false;
        }
    }
    if (!_repeatPattern.isNull()) {
        QVector<ScriptObjectPointerList> lists(_repeatVariableCount);
        for (; idx < size; idx++) {
            if (!_repeatPattern->matches(contents.at(idx), scope, variables)) {
                break;
            }
            for (int ii = 0; ii < _repeatVariableCount; ii++) {
                lists[ii].append(variables.at(_repeatVariableIdx + ii));
            }
        }
        if (size - idx < postcount) {
            return false;
        }
        for (int ii = 0; ii < _repeatVariableCount; ii++) {
            variables[ii] = ScriptObjectPointer(new List(lists.at(ii)));
        }
        for (int ii = 0; ii < postcount && idx < size; ii++, idx++) {
            if (!_postRepeatPatterns.at(ii)->matches(contents.at(idx), scope, variables)) {
                return false;
            }
        }
    }
    if (_restPattern.isNull()) {
        return idx == size;
    }
    ScriptObjectPointerList rest;
    for (; idx < size; idx++) {
        rest.append(contents.at(idx));
    }
    return _restPattern->matches(ScriptObjectPointer(new List(rest)), scope, variables);
}

DatumTemplate::DatumTemplate (const ScriptObjectPointer& datum) :
    _datum(datum)
{
}

ScriptObjectPointer DatumTemplate::generate (QVector<ScriptObjectPointer>& variables)
{
    return _datum;
}

VariableTemplate::VariableTemplate (int index) :
    _index(index)
{
}

ScriptObjectPointer VariableTemplate::generate (QVector<ScriptObjectPointer>& variables)
{
    return variables.at(_index);
}

ListTemplate::ListTemplate (
        const QVector<Subtemplate>& subtemplates, const TemplatePointer& restTemplate) :
    _subtemplates(subtemplates),
    _restTemplate(restTemplate)
{
}

ScriptObjectPointer ListTemplate::generate (QVector<ScriptObjectPointer>& variables)
{
    ScriptObjectPointerList list;
    
    if (!_restTemplate.isNull()) {
        ScriptObjectPointer rest = _restTemplate->generate(variables);
        if (rest->type() == ScriptObject::ListType) {
            List* rlist = static_cast<List*>(rest.data());
            list += rlist->contents();
        } else {
            list.append(rest);
        }
    }
    return ScriptObjectPointer(new List(list));
}
