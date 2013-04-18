//
// $Id$

#include <QtDebug>

#include "script/MacroTransformer.h"

class RepeatGroup;

/** A shared pointer to a repeat group. */
typedef QSharedPointer<RepeatGroup> RepeatGroupPointer;

/**
 * Represents a repeat group within a pattern.
 */
class RepeatGroup
{
public:

    /** The parent group. */
    RepeatGroupPointer parent;

    /** The index of the first variable involved in the repeat group. */
    int variableIdx;

    /** The number of variables involved in the repeat group. */
    int variableCount;
};

/** A variable mapping (index and repeat group). */
typedef QPair<int, RepeatGroupPointer> VariableMapping;

/**
 * Creates a pattern from the specified expression.
 */
static PatternPointer createPattern (
    ScriptObjectPointer expr, const QHash<QString, PatternPointer>& literals,
    QHash<QString, VariableMapping>& variables,
    const RepeatGroupPointer& parentGroup = RepeatGroupPointer())
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
            variables.insert(name, VariableMapping(index, parentGroup));
            return PatternPointer(new VariablePattern(index));
        }
        case ScriptObject::PairType: {
            Pair* pair = static_cast<Pair*>(expr.data());
            QVector<PatternPointer> preRepeatPatterns;
            PatternPointer repeatPattern;
            RepeatGroupPointer repeatGroup(new RepeatGroup());
            QVector<PatternPointer> postRepeatPatterns;
            PatternPointer restPattern;
            for (Pair* rest = pair;; ) {
                ScriptObjectPointer cdr = rest->cdr();
                if (cdr->type() == ScriptObject::PairType) {
                    Pair* pcdr = static_cast<Pair*>(cdr.data());
                    ScriptObjectPointer cadr = pcdr->car();
                    if (cadr->type() == ScriptObject::SymbolType) {
                        Symbol* symbol = static_cast<Symbol*>(cadr.data());
                        if (symbol->name() == "...") {
                            repeatGroup->parent = parentGroup;
                            repeatGroup->variableIdx = variables.size();
                            repeatPattern = createPattern(
                                rest->car(), literals, variables, repeatGroup);
                            repeatGroup->variableCount =
                                variables.size() - repeatGroup->variableIdx;

                            for (ScriptObjectPointer cddr = pcdr->cdr();; ) {
                                switch (cddr->type()) {
                                    case ScriptObject::PairType: {
                                        Pair* pcddr = static_cast<Pair*>(cddr.data());
                                        postRepeatPatterns.append(createPattern(
                                            pcddr->car(), literals, variables, parentGroup));
                                        cddr = pcddr->cdr();
                                        break;
                                    }
                                    case ScriptObject::NullType:
                                        goto outerBreak;

                                    default:
                                        restPattern = createPattern(
                                            cddr, literals, variables, parentGroup);
                                        goto outerBreak;
                                }
                            }
                        }
                    }
                }
                preRepeatPatterns.append(createPattern(
                    rest->car(), literals, variables, parentGroup));

                switch (cdr->type()) {
                    case ScriptObject::PairType:
                        rest = static_cast<Pair*>(cdr.data());
                        break;

                    case ScriptObject::NullType:
                        goto outerBreak;

                    default:
                        restPattern = createPattern(cdr, literals, variables, parentGroup);
                        goto outerBreak;
                }
            }
            outerBreak:

            return PatternPointer(new ListPattern(
                preRepeatPatterns, repeatPattern, repeatGroup->variableIdx,
                repeatGroup->variableCount, postRepeatPatterns, restPattern));
        }
        case ScriptObject::VectorType: {
            Vector* list = static_cast<Vector*>(expr.data());
            const ScriptObjectPointerVector& contents = list->contents();
            QVector<PatternPointer> preRepeatPatterns;
            PatternPointer repeatPattern;
            RepeatGroupPointer repeatGroup(new RepeatGroup());
            QVector<PatternPointer> postRepeatPatterns;
            for (int ii = 0, nn = contents.size(); ii < nn; ii++) {
                if (ii + 1 < nn) {
                    ScriptObjectPointer next = contents.at(ii + 1);
                    if (next->type() == ScriptObject::SymbolType) {
                        Symbol* symbol = static_cast<Symbol*>(next.data());
                        if (symbol->name() == "...") {
                            repeatGroup->parent = parentGroup;
                            repeatGroup->variableIdx = variables.size();
                            repeatPattern = createPattern(
                                contents.at(ii), literals, variables, repeatGroup);
                            repeatGroup->variableCount =
                                variables.size() - repeatGroup->variableIdx;
                            for (ii += 2; ii < nn; ii++) {
                                postRepeatPatterns.append(createPattern(
                                    contents.at(ii), literals, variables, parentGroup));
                            }
                            break;
                        }
                    }
                }
                preRepeatPatterns.append(createPattern(
                    contents.at(ii), literals, variables, parentGroup));
            }
            return PatternPointer(new VectorPattern(
                preRepeatPatterns, repeatPattern, repeatGroup->variableIdx,
                repeatGroup->variableCount, postRepeatPatterns));
        }
        default:
            return PatternPointer(new ConstantPattern(expr));
    }
}

/**
 * Assigns the repeat group if possible and not yet assigned.  Throws a ScriptError with the
 * supplied position if the two repeat groups aren't compatible.
 */
void maybeAssignGroup (
    RepeatGroupPointer& group, const RepeatGroupPointer& ngroup, const ScriptPosition& pos)
{
    if (!ngroup.isNull()) {
        if (group.isNull()) {
            group = ngroup;

        } else if (group != ngroup) {
            throw ScriptError("Invalid repeat group.", pos);
        }
    }
}

/**
 * Creates a template from the specified expression.
 *
 * @param ellipseIdents if true, treat ellipses as normal identifiers.
 */
static TemplatePointer createTemplate (
    ScriptObjectPointer expr, Scope* scope, const QHash<QString, VariableMapping>& variables,
    bool ellipseIdents, RepeatGroupPointer& repeatGroup)
{
    switch (expr->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            const QString& name = symbol->name();
            QHash<QString, VariableMapping>::const_iterator it = variables.constFind(name);
            if (it == variables.constEnd()) {
                return TemplatePointer(new SymbolTemplate(scope, expr));
            }
            maybeAssignGroup(repeatGroup, it.value().second, symbol->position());
            return TemplatePointer(new VariableTemplate(it.value().first));
        }
        case ScriptObject::PairType: {
            Pair* pair = static_cast<Pair*>(expr.data());
            ScriptObjectPointer car = pair->car();
            if (car->type() == ScriptObject::SymbolType) {
                Symbol* symbol = static_cast<Symbol*>(car.data());
                if (symbol->name() == "..." && !ellipseIdents) {
                    Pair* cdr = requirePair(pair->cdr(), "Invalid template.", pair->position());
                    requireNull(cdr->cdr(), "Invalid template.", pair->position());
                    return createTemplate(cdr->car(), scope, variables, true, repeatGroup);
                }
            }
            QVector<Subtemplate> subtemplates;
            TemplatePointer restTemplate;

            for (Pair* rest = pair;; ) {
                RepeatGroupPointer subRepeatGroup;
                TemplatePointer templ = createTemplate(
                    rest->car(), scope, variables, ellipseIdents, subRepeatGroup);
                int repeatVariableIdx;
                int repeatVariableCount;
                int repeatDepth = 0;
                if (!ellipseIdents) {
                    forever {
                        ScriptObjectPointer cdr = rest->cdr();
                        switch (cdr->type()) {
                            case ScriptObject::PairType: {
                                Pair* pcdr = static_cast<Pair*>(cdr.data());
                                ScriptObjectPointer cadr = pcdr->car();
                                if (cadr->type() != ScriptObject::SymbolType) {
                                    goto repeatBreak;
                                }
                                Symbol* symbol = static_cast<Symbol*>(cadr.data());
                                if (symbol->name() != "...") {
                                    goto repeatBreak;
                                }
                                if (subRepeatGroup.isNull()) {
                                    throw ScriptError("Invalid repeat depth.", symbol->position());
                                }
                                repeatVariableIdx = subRepeatGroup->variableIdx;
                                repeatVariableCount = subRepeatGroup->variableCount;
                                subRepeatGroup = subRepeatGroup->parent;
                                repeatDepth++;
                                rest = pcdr;
                                break;
                            }
                            default:
                                goto repeatBreak;
                        }
                    }
                    repeatBreak: ;
                }
                maybeAssignGroup(repeatGroup, subRepeatGroup, pair->position());
                subtemplates.append(Subtemplate(
                    repeatVariableIdx, repeatVariableCount, repeatDepth, templ));

                ScriptObjectPointer cdr = rest->cdr();
                switch (cdr->type()) {
                    case ScriptObject::PairType:
                        rest = static_cast<Pair*>(cdr.data());
                        break;

                    case ScriptObject::NullType:
                        goto outerBreak;

                    default:
                        restTemplate = createTemplate(
                            cdr, scope, variables, ellipseIdents, repeatGroup);
                        goto outerBreak;
                }
            }
            outerBreak:

            return TemplatePointer(new ListTemplate(subtemplates, restTemplate));
        }
        case ScriptObject::VectorType: {
            Vector* list = static_cast<Vector*>(expr.data());
            const ScriptObjectPointerVector& contents = list->contents();
            QVector<Subtemplate> subtemplates;
            for (int ii = 0, nn = contents.size(); ii < nn; ii++) {
                ScriptObjectPointer element = contents.at(ii);
                if (element->type() == ScriptObject::SymbolType) {
                    Symbol* symbol = static_cast<Symbol*>(element.data());
                    const QString& name = symbol->name();
                    if (name == "..." && !ellipseIdents) {
                        if (nn != 2) {
                            throw ScriptError("Invalid template.", list->position());
                        }
                        return createTemplate(contents.at(1), scope, variables, true, repeatGroup);
                    }
                }
                RepeatGroupPointer subRepeatGroup;
                TemplatePointer templ = createTemplate(
                    contents.at(ii), scope, variables, ellipseIdents, subRepeatGroup);
                int repeatVariableIdx;
                int repeatVariableCount;
                int repeatDepth = 0;
                if (!ellipseIdents) {
                    for (ii++; ii < nn; ii++) {
                        ScriptObjectPointer nelement = contents.at(ii);
                        if (nelement->type() != ScriptObject::SymbolType) {
                            break;
                        }
                        Symbol* symbol = static_cast<Symbol*>(nelement.data());
                        if (symbol->name() != "...") {
                            break;
                        }
                        if (subRepeatGroup.isNull()) {
                            throw ScriptError("Invalid repeat depth.", symbol->position());
                        }
                        repeatVariableIdx = subRepeatGroup->variableIdx;
                        repeatVariableCount = subRepeatGroup->variableCount;
                        subRepeatGroup = subRepeatGroup->parent;
                        repeatDepth++;
                    }
                    ii--;
                }
                maybeAssignGroup(repeatGroup, subRepeatGroup, list->position());
                subtemplates.append(Subtemplate(
                    repeatVariableIdx, repeatVariableCount, repeatDepth, templ));
            }
            return TemplatePointer(new VectorTemplate(subtemplates));
        }
        default:
            return TemplatePointer(new DatumTemplate(expr));
    }
}

/**
 * Creates a template from the specified expression.
 */
static TemplatePointer createTemplate (
    ScriptObjectPointer expr, Scope* scope, const QHash<QString, VariableMapping>& variables)
{
    RepeatGroupPointer repeatGroup;
    TemplatePointer templ = createTemplate(expr, scope, variables, false, repeatGroup);
    if (!repeatGroup.isNull()) {
        Datum* datum = static_cast<Datum*>(expr.data());
        throw ScriptError("Invalid repeat depth.", datum->position());
    }
    return templ;
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
        case ScriptObject::PairType: {
            Pair* pair = static_cast<Pair*>(expr.data());
            Symbol* symbol = requireSymbol(pair->car(), "Invalid macro transformer.",
                pair->position());
            Pair* cdr = requirePair(pair->cdr(), "Invalid macro transformer.", pair->position());
            const QString& name = symbol->name();
            if (name == "syntax-rules") {
                QHash<QString, PatternPointer> literals;
                for (ScriptObjectPointer rest = cdr->car();; ) {
                    switch (rest->type()) {
                        case ScriptObject::PairType: {
                            Pair* rpair = static_cast<Pair*>(rest.data());
                            Symbol* literal = requireSymbol(rpair->car(), "Invalid literal.",
                                rpair->position());
                            const QString& lname = literal->name();
                            ScriptObjectPointer binding = scope->resolve(lname);
                            literals.insert(lname, PatternPointer(binding.isNull() ?
                                (Pattern*)new UnboundLiteralPattern(lname) :
                                (Pattern*)new BoundLiteralPattern(binding)));
                            rest = rpair->cdr();
                            break;
                        }
                        case ScriptObject::NullType:
                           goto literalBreak;

                        default:
                            throw ScriptError("Invalid syntax rules.", pair->position());
                    }
                }
                literalBreak:

                QVector<PatternTemplate> patternTemplates;
                for (ScriptObjectPointer rest = cdr->cdr();; ) {
                    switch (rest->type()) {
                        case ScriptObject::PairType: {
                            Pair* rpair = static_cast<Pair*>(rest.data());
                            Pair* rule = requirePair(rpair->car(), "Invalid syntax rule.",
                                rpair->position());
                            Pair* rulecdr = requirePair(rule->cdr(), "Invalid syntax rule.",
                                rpair->position());
                            requireNull(rulecdr->cdr(), "Invalid syntax rule.", rpair->position());

                            QHash<QString, VariableMapping> variables;
                            PatternPointer pattern = createPattern(rule->car(), literals, variables);
                            patternTemplates.append(PatternTemplate(variables.size(), pattern,
                                createTemplate(rulecdr->car(), scope, variables)));
                            rest = rpair->cdr();
                            break;
                        }
                        case ScriptObject::NullType:
                            goto templateBreak;

                        default:
                            throw ScriptError("Invalid syntax rules.", pair->position());
                    }
                }
                templateBreak:

                return ScriptObjectPointer(new SyntaxRules(patternTemplates));

            } else if (name == "identifier-syntax") {
                ScriptObjectPointer cddr = cdr->cdr();
                switch (cddr->type()) {
                    case ScriptObject::NullType:
                        return ScriptObjectPointer(new IdentifierSyntax(
                            createTemplate(cdr->car(), scope, QHash<QString, VariableMapping>()),
                            PatternTemplate()));

                    case ScriptObject::PairType: {
                        Pair* pcddr = static_cast<Pair*>(cddr.data());
                        requireNull(pcddr->cdr(), "Invalid identifier syntax.", pair->position());
                        Pair* cadr = requirePair(cdr->car(), "Invalid identifier syntax.",
                            pair->position());
                        requireSymbol(cadr->car(), "Invalid rule.", cadr->position());
                        Pair* cdadr = requirePair(cadr->cdr(), "Invalid rule.", cadr->position());
                        requireNull(cdadr->cdr(), "Invalid rule.", cadr->position());
                        TemplatePointer templ = createTemplate(cdadr->car(), scope,
                            QHash<QString, VariableMapping>());

                        Pair* caddr = requirePair(pcddr->car(), "Invalid identifier syntax.",
                            pair->position());
                        Pair* cdaddr = requirePair(caddr->cdr(), "Invalid rule.",
                            caddr->position());
                        requireNull(cdaddr->cdr(), "Invalid rule.", caddr->position());
                        Pair* caaddr = requirePair(caddr->car(), "Invalid rule.",
                            caddr->position());
                        Pair* cdaaddr = requirePair(caaddr->cdr(), "Invalid rule.",
                            caddr->position());
                        Pair* cddaaddr = requirePair(cdaaddr->cdr(), "Invalid rule.",
                            caddr->position());
                        requireNull(cddaaddr->cdr(), "Invalid rule.", caddr->position());
                        Symbol* set = requireSymbol(caaddr->car(), "Invalid rule.",
                            caddr->position());
                        if (set->name() != "set!") {
                            throw ScriptError("Invalid rule.", caddr->position());
                        }
                        requireSymbol(cdaaddr->car(), "Invalid rule.", caddr->position());

                        QHash<QString, VariableMapping> variables;
                        PatternPointer pattern = createPattern(
                            cddaaddr->car(), QHash<QString, PatternPointer>(), variables);
                        return ScriptObjectPointer(new IdentifierSyntax(templ, PatternTemplate(
                            variables.size(), pattern, createTemplate(
                                cdaddr->car(), scope, variables))));
                    }
                    default:
                        throw ScriptError("Invalid identifier syntax.", pair->position());
                }
            } else {
                throw ScriptError("Invalid macro transformer.", pair->position());
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
    ScriptObjectPointer form, Scope* scope, ScriptObjectPointerVector& variables) const
{
    return equivalent(form, _constant);
}

VariablePattern::VariablePattern (int index) :
    _index(index)
{
}

bool VariablePattern::matches (
    ScriptObjectPointer form, Scope* scope, ScriptObjectPointerVector& variables) const
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
    ScriptObjectPointer form, Scope* scope, ScriptObjectPointerVector& variables) const
{
    if (form->type() == ScriptObject::SymbolType) {
        Symbol* symbol = static_cast<Symbol*>(form.data());
        form = scope->resolve(symbol->name());
        if (form.isNull()) {
            return false;
        }
    }
    return equivalent(form, _binding);
}

UnboundLiteralPattern::UnboundLiteralPattern (const QString& name) :
    _name(name)
{
}

bool UnboundLiteralPattern::matches (
    ScriptObjectPointer form, Scope* scope, ScriptObjectPointerVector& variables) const
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
    ScriptObjectPointer form, Scope* scope, ScriptObjectPointerVector& variables) const
{
    if (form->type() != ScriptObject::PairType) {
        return false;
    }
    Pair* pair = static_cast<Pair*>(form.data());
    for (int ii = 0, nn = _preRepeatPatterns.size(); ii < nn; ii++) {
        if (!_preRepeatPatterns.at(ii)->matches(pair->car(), scope, variables)) {
            return false;
        }
        ScriptObjectPointer cdr = pair->cdr();
        switch (cdr->type()) {
            case ScriptObject::PairType:
                pair = static_cast<Pair*>(cdr.data());
                break;

            case ScriptObject::NullType:
                for (int jj = 0; jj < _repeatVariableCount; jj++) {
                    variables[_repeatVariableIdx + jj] =
                        Vector::instance(ScriptObjectPointerVector());
                }
                return ii == nn - 1 && _postRepeatPatterns.isEmpty() && _restPattern.isNull();

            default:
                for (int jj = 0; jj < _repeatVariableCount; jj++) {
                    variables[_repeatVariableIdx + jj] =
                        Vector::instance(ScriptObjectPointerVector());
                }
                return ii == nn - 1 && _postRepeatPatterns.isEmpty() && !_restPattern.isNull() &&
                    _restPattern->matches(cdr, scope, variables);
        }
    }
    if (!_repeatPattern.isNull()) {
        QVector<ScriptObjectPointerVector> lists(_repeatVariableCount);
        forever {
            if (!_repeatPattern->matches(pair->car(), scope, variables)) {
                break;
            }
            for (int ii = 0; ii < _repeatVariableCount; ii++) {
                lists[ii].append(variables.at(_repeatVariableIdx + ii));
            }

            ScriptObjectPointer cdr = pair->cdr();
            switch (cdr->type()) {
                case ScriptObject::PairType:
                    pair = static_cast<Pair*>(cdr.data());
                    break;

                case ScriptObject::NullType:
                    for (int ii = 0; ii < _repeatVariableCount; ii++) {
                        variables[_repeatVariableIdx + ii] = Vector::instance(lists.at(ii));
                    }
                    return _postRepeatPatterns.isEmpty() && _restPattern.isNull();

                default:
                    for (int ii = 0; ii < _repeatVariableCount; ii++) {
                        variables[_repeatVariableIdx + ii] = Vector::instance(lists.at(ii));
                    }
                    return _postRepeatPatterns.isEmpty() && !_restPattern.isNull() &&
                        _restPattern->matches(cdr, scope, variables);
            }
        }

        for (int ii = 0; ii < _repeatVariableCount; ii++) {
            variables[_repeatVariableIdx + ii] = Vector::instance(lists.at(ii));
        }
        for (int ii = 0, nn = _postRepeatPatterns.size(); ii < nn; ii++) {
            if (!_postRepeatPatterns.at(ii)->matches(pair->car(), scope, variables)) {
                return false;
            }
            ScriptObjectPointer cdr = pair->cdr();
            switch (cdr->type()) {
                case ScriptObject::PairType:
                    pair = static_cast<Pair*>(cdr.data());
                    break;

                case ScriptObject::NullType:
                    return ii == nn - 1 && _restPattern.isNull();

                default:
                    return ii == nn - 1 && !_restPattern.isNull() &&
                        _restPattern->matches(cdr, scope, variables);
            }
        }
    }
    return false;
}

VectorPattern::VectorPattern (const QVector<PatternPointer>& preRepeatPatterns,
        const PatternPointer& repeatPattern, int repeatVariableIdx, int repeatVariableCount,
        const QVector<PatternPointer>& postRepeatPatterns) :
    _preRepeatPatterns(preRepeatPatterns),
    _repeatPattern(repeatPattern),
    _repeatVariableIdx(repeatVariableIdx),
    _repeatVariableCount(repeatVariableCount),
    _postRepeatPatterns(postRepeatPatterns)
{
}

bool VectorPattern::matches (
    ScriptObjectPointer form, Scope* scope, ScriptObjectPointerVector& variables) const
{
    if (form->type() != ScriptObject::VectorType) {
        return false;
    }
    Vector* list = static_cast<Vector*>(form.data());
    const ScriptObjectPointerVector& contents = list->contents();
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
        QVector<ScriptObjectPointerVector> lists(_repeatVariableCount);
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
            variables[_repeatVariableIdx + ii] = Vector::instance(lists.at(ii));
        }
        for (int ii = 0; ii < postcount && idx < size; ii++, idx++) {
            if (!_postRepeatPatterns.at(ii)->matches(contents.at(idx), scope, variables)) {
                return false;
            }
        }
    }
    return idx == size;
}

DatumTemplate::DatumTemplate (const ScriptObjectPointer& datum) :
    _datum(datum)
{
}

ScriptObjectPointer DatumTemplate::generate (ScriptObjectPointerVector& variables) const
{
    return _datum;
}

SymbolTemplate::SymbolTemplate (Scope* scope, const ScriptObjectPointer& datum) :
    _scope(scope),
    _datum(datum)
{
}

ScriptObjectPointer SymbolTemplate::generate (ScriptObjectPointerVector& variables) const
{
    Symbol* symbol = static_cast<Symbol*>(_datum.data());
    ScriptObjectPointer binding = _scope->resolve(symbol->name());
    return binding.isNull() ? _datum : binding;
}

VariableTemplate::VariableTemplate (int index) :
    _index(index)
{
}

ScriptObjectPointer VariableTemplate::generate (ScriptObjectPointerVector& variables) const
{
    return variables.at(_index);
}

ScriptObjectPointer ListBuilder::list (const ScriptObjectPointer& rest)
{
    if (_head.isNull()) {
        return rest;
    }
    static_cast<Pair*>(_tail.data())->setCdr(rest);
    return _head;
}

void ListBuilder::append (const ScriptObjectPointer& element)
{
    ScriptObjectPointer ntail(new Pair(element));
    if (_head.isNull()) {
        _head = _tail = ntail;

    } else {
        static_cast<Pair*>(_tail.data())->setCdr(ntail);
        _tail = ntail;
    }
}

void VectorBuilder::append (const ScriptObjectPointer& element)
{
    _vector.append(element);
}

Subtemplate::Subtemplate (
        int repeatVariableIdx, int repeatVariableCount,
        int repeatDepth, const TemplatePointer& templ) :
    _repeatVariableIdx(repeatVariableIdx),
    _repeatVariableCount(repeatVariableCount),
    _repeatDepth(repeatDepth),
    _template(templ)
{
}

Subtemplate::Subtemplate ()
{
}

void Subtemplate::generate (ScriptObjectPointerVector& variables, ObjectBuilder* builder) const
{
    generate(variables, builder, _repeatDepth);
}

void Subtemplate::generate (
    ScriptObjectPointerVector& variables, ObjectBuilder* builder, int depth) const
{
    if (depth == 0) {
        builder->append(_template->generate(variables));
        return;
    }
    ScriptObjectPointerVector wrapped(_repeatVariableCount);
    for (int ii = 0; ii < _repeatVariableCount; ii++) {
        wrapped[ii] = variables[_repeatVariableIdx + ii];
    }

    int count = static_cast<Vector*>(wrapped[0].data())->contents().size();
    for (int ii = 0; ii < count; ii++) {
        for (int jj = 0; jj < _repeatVariableCount; jj++) {
            Vector* list = static_cast<Vector*>(wrapped[jj].data());
            variables[_repeatVariableIdx + jj] = list->contents().at(ii);
        }
        generate(variables, builder, depth - 1);
    }
    for (int ii = 0; ii < _repeatVariableCount; ii++) {
        variables[_repeatVariableIdx + ii] = wrapped.at(ii);
    }
}

ListTemplate::ListTemplate (
        const QVector<Subtemplate>& subtemplates, const TemplatePointer& restTemplate) :
    _subtemplates(subtemplates),
    _restTemplate(restTemplate)
{
}

ScriptObjectPointer ListTemplate::generate (ScriptObjectPointerVector& variables) const
{
    ListBuilder builder;
    foreach (const Subtemplate& subtemplate, _subtemplates) {
        subtemplate.generate(variables, &builder);
    }
    return builder.list(_restTemplate.isNull() ?
        Null::instance() : _restTemplate->generate(variables));
}

VectorTemplate::VectorTemplate (const QVector<Subtemplate>& subtemplates) :
    _subtemplates(subtemplates)
{
}

ScriptObjectPointer VectorTemplate::generate (ScriptObjectPointerVector& variables) const
{
    VectorBuilder builder;
    foreach (const Subtemplate& subtemplate, _subtemplates) {
        subtemplate.generate(variables, &builder);
    }
    return Vector::instance(builder.vector());
}
