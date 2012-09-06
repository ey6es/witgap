//
// $Id$

#include "script/MacroTransformer.h"

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
                
//                return ScriptObjectPointer(new SyntaxRules());
                
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

