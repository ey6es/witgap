//
// $Id$

#ifndef GLOBALS
#define GLOBALS

class Scope;

/**
 * Returns a pointer to the global scope.
 */
Scope* globalScope ();

/**
 * Returns a pointer to the list procedure.
 */
const ScriptObjectPointer& listProcedure ();

/**
 * Returns a pointer to the list append procedure.
 */
const ScriptObjectPointer& appendProcedure ();

/**
 * Returns a pointer to the vector procedure.
 */
const ScriptObjectPointer& vectorProcedure ();

/**
 * Returns a pointer to the vector append procedure.
 */
const ScriptObjectPointer& vectorAppendProcedure();

#endif // GLOBALS
