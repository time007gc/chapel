#include "astutil.h"
#include "expr.h"
#include "optimizations.h"
#include "passes.h"
#include "stmt.h"
#include "stringutil.h"
#include "symbol.h"
#include "type.h"


static Type* getWrapRecordBaseType(Type* type);


//
// removes _array and _domain wrapper records
//
void
removeWrapRecords() {
  //
  // do not remove wrap records if dead code elimination is disabled
  // (or weakened because inlining or copy propagation is disabled)
  // because code associated with accesses to the removed
  // _valueType field will remain
  //
  if (fNoDeadCodeElimination || fNoInline || fNoCopyPropagation)
    return;

  //
  // replace use of _valueType field with type
  //
  forv_Vec(CallExpr, call, gCallExprs) {
    if (call->isPrimitive(PRIMITIVE_PRIVATE_GET_CLASS)) {
      call->get(1)->replace(new SymExpr(call->get(1)->typeInfo()->symbol));
    }
  }

  //
  // remove defs of _valueType field
  //
  forv_Vec(CallExpr, call, gCallExprs) {
    if (call->isPrimitive(PRIMITIVE_SET_MEMBER) ||
        call->isPrimitive(PRIMITIVE_GET_MEMBER) ||
        call->isPrimitive(PRIMITIVE_GET_MEMBER_VALUE)) {
      if (SymExpr* se = toSymExpr(call->get(2))) {
        if (!strcmp(se->var->name, "_valueType")) {
          se->getStmtExpr()->remove();
        }
      }
    }
  }

  //
  // remove _valueType fields
  //
  forv_Vec(ClassType, ct, gClassTypes) {
    for_fields(field, ct) {
      if (!strcmp(field->name, "_valueType"))
        field->defPoint->remove();
    }
  }

  //
  // remove formals for _valueType fields in constructors
  //
  compute_call_sites();
  forv_Vec(FnSymbol, fn, gFnSymbols) {
    for_formals(formal, fn) {
      if (!strcmp(formal->name, "_valueType")) {
        forv_Vec(CallExpr, call, *fn->calledBy) {
          formal_to_actual(call, formal)->remove();
        }
        formal->defPoint->remove();
      }        
    }
  }

  //
  // replace accesses of _value with wrap record
  //
  forv_Vec(CallExpr, call, gCallExprs) {
    if (call->isPrimitive(PRIMITIVE_SET_MEMBER)) {
      if (SymExpr* se = toSymExpr(call->get(1))) {
        if (se->var->type->symbol->hasFlag(FLAG_ARRAY) ||
            se->var->type->symbol->hasFlag(FLAG_DOMAIN)) {
          call->primitive = primitives[PRIMITIVE_MOVE];
          call->get(2)->remove();
        }
      }
    } else if (call->isPrimitive(PRIMITIVE_GET_MEMBER)) {
      if (SymExpr* se = toSymExpr(call->get(1))) {
        if (se->var->type->symbol->hasFlag(FLAG_ARRAY) ||
            se->var->type->symbol->hasFlag(FLAG_DOMAIN)) {
          call->primitive = primitives[PRIMITIVE_SET_REF];
          call->get(2)->remove();
        }
      }
    } else if (call->isPrimitive(PRIMITIVE_GET_MEMBER_VALUE)) {
      if (SymExpr* se = toSymExpr(call->get(1))) {
        if (se->var->type->symbol->hasFlag(FLAG_ARRAY) ||
            se->var->type->symbol->hasFlag(FLAG_DOMAIN)) {
          call->replace(se->remove());
        }
      }
    }
  }

  //
  // scalar replace wrap records
  //
  forv_Vec(VarSymbol, var, gVarSymbols) {
    if (Type* type = getWrapRecordBaseType(var->type))
      if (!var->defPoint->parentSymbol->hasFlag(FLAG_REF)) // refs second
        var->type = type;
  }
  forv_Vec(ArgSymbol, arg, gArgSymbols) {
    if (Type* type = getWrapRecordBaseType(arg->type)) {
      arg->intent = INTENT_BLANK; // see test/arrays/deitz/test_out_array
      arg->type = type;
    }
  }
  forv_Vec(FnSymbol, fn, gFnSymbols) {
    if (Type* type = getWrapRecordBaseType(fn->retType))
      fn->retType = type;
  }
  forv_Vec(ClassType, ct, gClassTypes) {
    if (ct->symbol->hasFlag(FLAG_REF))
      if (Symbol* var = ct->getField(1))
        if (Type* type = getWrapRecordBaseType(var->type))
          var->type = type;
  }

  //
  // fix array element type for arrays of arrays and arrays of domains
  //
  forv_Vec(ClassType, ct, gClassTypes) {
    if (ct->symbol->hasFlag(FLAG_DATA_CLASS)) {
      if (TypeSymbol* ts = toTypeSymbol(ct->substitutions.v[0].value)) {
        if (ts->hasFlag(FLAG_ARRAY) || ts->hasFlag(FLAG_DOMAIN)) {
          ct->substitutions.v[0].value = ts->type->getField("_value")->type->symbol;
        }
      }
    }
  }
}


static Type*
getWrapRecordBaseType(Type* type) {
  if (type->symbol->hasFlag(FLAG_ARRAY) ||
      type->symbol->hasFlag(FLAG_DOMAIN)) {
    return type->getField("_value")->type;
  } else if (type->symbol->hasFlag(FLAG_REF)) {
    Type* vt = type->getValueType();
    if (vt->symbol->hasFlag(FLAG_ARRAY) ||
        vt->symbol->hasFlag(FLAG_DOMAIN)) {
      return vt->getField("_value")->type->refType;
    }
  }
  return NULL;
}
