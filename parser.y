%{
// Paul Scherer, pscherer@ucsc.edu

// Dummy parser for scanner project.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token ROOT IDENT NUMBER
%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_INTCON TOK_CHARCON TOK_STRCON

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR

%right TOK_IF TOK_ELSE
%right '='
%left  TOK_EQ TOK_NE TOK_LE TOK_GE TOK_LT TOK_GT
%left  '+' '-'
%left  '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_ORD TOK_CHR
%left  TOK_CALL
%left  '[' '.'
%nonassoc TOK_NEW
%nonassoc TOK_PAREN

%start program

%%

start     : program             { yyparse_astree = $1; }
          ;
          
program   : program structdef   { $$ = adopt1 ($1, $2); }
          | program function    { $$ = adopt1 ($1, $2); }
          | program statement   { $$ = adopt1 ($1, $2); }
          | program error '}'   { $$ = $1; }
          | program error ';'   { $$ = $1; }
          | /* empty */         { $$ = new_parseroot(); }
          ;
        
structdef : TOK_STRUCT IDENT '{' declseq '}'
                                { $$ = adopt2 ($1, $2, $4); }
          ;

declseq   : declseq decl ';'    { $$ = adopt1 ($1, $2); }
          | /* empty */

decl      : type IDENT          { $$ = adopt1 ($1, $2); }
          ;

type      : basetype TOK_ARRAY  { $$ = adopt1sym ($1, $2, TOK_ARRAY); }
          | basetype            { $$ = $1; }
          ; 

basetype  : TOK_VOID            { $$ = $1; }
          | TOK_BOOL            { $$ = $1; }
          | TOK_CHAR            { $$ = $1; }
          | TOK_INT             { $$ = $1; }
          | TOK_STRING          { $$ = $1; }
          | IDENT               { $$ = $1; }
          ;
          
function  : type IDENT '(' ')' block
                                { $$ = adopt2 ($1, $2, $5); }
          | type IDENT '(' decl funcseq ')' block
                                { $1 = adopt2 ($1, $4, $5); }
                                { $$ = adopt2 ($1, $2, $7); }
          ;

funcseq   : funcseq ',' decl    { $$ = adopt1 ($1, $3); }
          | /* empty */
          ;

block     : '{' stmtseq '}'     { $$ = $1; }
          | ';'                 { $$ = $1; }
          ;

stmtseq   : stmtseq statement   { $$ = adopt1 ($1, $2); }
          | /* empty */
          ;

statement : block               { $$ = $1; }
          | vardecl             { $$ = $1; }
          | while               { $$ = $1; }
          | ifelse              { $$ = $1; }
          | return              { $$ = $1; }
          | expr ';'            { $$ = $1; }
          ;

vardecl   : type IDENT '=' expr ';'
                                { $$ = adopt2 ($1, $2, $4); }
          ;

while     : TOK_WHILE '(' expr ')' statement
                                { $$ = adopt2 ($1, $3, $5); }
          ;

ifelse    : TOK_IF '('expr')' statement
            TOK_IF '('expr')' statement TOK_ELSE statement
                                { $1 = adopt2 ($1, $2, $4); }
          ;

return    : TOK_RETURN expr ';'
          | TOK_RETURN ';'
          ;

expr      : binop
          | unop
          | allocator
          | call
          | '(' expr ')' %prec TOK_PAREN
          | variable
          | constant
          ;

binop     : expr '=' expr           { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_EQ expr        { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_NE expr        { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_LE expr        { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_GE expr        { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_LT expr        { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_GT expr        { $$ = adopt2 ($2, $1, $3); }
          | expr '+' expr           { $$ = adopt2 ($2, $1, $3); }
          | expr '-' expr           { $$ = adopt2 ($2, $1, $3); }
          | expr '*' expr           { $$ = adopt2 ($2, $1, $3); }
          | expr '/' expr           { $$ = adopt2 ($2, $1, $3); }
          | expr '%' expr           { $$ = adopt2 ($2, $1, $3); }
          ;
          
unop      : '+' expr %prec TOK_POS
          | '-' expr %prec TOK_NEG
          | '!' expr
          | TOK_ORD expr
          | TOK_CHR expr
          ;

allocator : TOK_NEW basetype '(' expr ')'
          | TOK_NEW basetype '(' ')'
          | TOK_NEW basetype '[' expr ']'
          ;

call      : IDENT '(' ')' %prec TOK_CALL
          | IDENT '(' expr exprseq ')' %prec TOK_CALL
          ;

exprseq   : exprseq ',' expr
          | /* empty */
          ;

variable  : IDENT                           { $$ = $1; }
          | expr '[' expr ']' %prec TOK_NEWARRAY
          | expr '.' IDENT
          ;

constant  : TOK_INTCON              { $$ = $1; }
          | TOK_STRCON              { $$ = $1; }
          | TOK_FALSE               { $$ = $1; }
          | TOK_TRUE                { $$ = $1; }
          | TOK_NULL                { $$ = $1; }
          ;
%%

const char *get_yytname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != NULL);
   return result;
}
