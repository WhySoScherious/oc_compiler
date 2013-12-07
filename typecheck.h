/*
 * typecheck.h
 *
 *  Created on: Dec 6, 2013
 *      Author: Paul
 */

#ifndef TYPECHECK_H_
#define TYPECHECK_H_

void typecheck_rec (astree* node, SymbolTable* types,
      SymbolTable* global, int depth);

string check_expr (astree* node, SymbolTable* types,
      SymbolTable* global);

#endif /* TYPECHECK_H_ */
