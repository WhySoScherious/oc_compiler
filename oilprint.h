/*
 * oilprint.h
 *
 *  Created on: Dec 5, 2013
 *      Author: Paul
 */

#ifndef OILPRINT_H_
#define OILPRINT_H_

void generate_oil (FILE* outfile, astree* yyparse_astree,
      SymbolTable* global, SymbolTable* types);

#endif /* OILPRINT_H_ */
