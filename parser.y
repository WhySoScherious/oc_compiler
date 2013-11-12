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

%token ROOT IDENT NUMBER TOK_FUNCTION TOK_PROTOTYPE
%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_INTCON TOK_CHARCON TOK_STRCON TOK_NEWSTRING

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_DECLID TOK_RETURNVOID
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_INDEX TOK_VARDECL TOK_PARAMLIST

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
          | program prototype   { $$ = adopt1 ($1, $2); }
          | program statement   { $$ = adopt1 ($1, $2); }
          | program error '}'   { $$ = $1; }
          | program error ';'   { $$ = $1; }
          | /* empty */         { $$ = new_parseroot(); }
          ;
        
structdef : TOK_STRUCT IDENT '{' declseq '}'
                                { $2->symbol = TOK_TYPEID;
                                  $$ = adopt2 ($1, $2, $4); }
          ;

declseq   : declseq decl ';'    { $$ = adopt1 ($1, $2); }
          | /* empty */
          ;

decl      : type IDENT          { $2->symbol = TOK_DECLID;
                                  $$ = adopt1 ($1, $2); }
          ;

type      : basetype TOK_ARRAY  { $$ = adopt1 ($2, $1); }
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
                              { astree* temp = new_astree (TOK_FUNCTION,
                                $1->filenr, $1->linenr, $1->offset,
                                "function");
                                $2->symbol = TOK_DECLID;
                                $1 = adopt2 ($1, $2, $5);
                                $$ = adopt1 (temp, $1); }
          | type IDENT '(' decl funcseq ')' block
                              { astree* temp = new_astree (TOK_FUNCTION,
                                $1->filenr, $1->linenr, $1->offset,
                                "function");
                                $2->symbol = TOK_DECLID;
                                $3->symbol = TOK_PARAMLIST;
                                $3 = adopt2 ($3, $4, $5);
                                $1 = adopt2 ($1, $2, $3);
                                $1 = adopt1 ($1, $7);
                                $$ = adopt1 (temp, $1); }
          ;

prototype  : type IDENT '(' ')' ';'
                             { astree* temp = new_astree (TOK_PROTOTYPE,
                               $1->filenr, $1->linenr, $1->offset,
                               "prototype");
                               $2->symbol = TOK_DECLID;
                               $1 = adopt1 ($1, $2);
                               $$ = adopt1 (temp, $1); }
          | type IDENT '(' decl funcseq ')' ';'
                             { astree* temp = new_astree (TOK_PROTOTYPE,
                               $1->filenr, $1->linenr, $1->offset,
                               "prototype");
                               $2->symbol = TOK_DECLID;
                               $3->symbol = TOK_PARAMLIST;
                               $3 = adopt2 ($3, $4, $5);
                               $1 = adopt2 ($1, $2, $3);
                               $$ = adopt1 (temp, $1); }
          ;

funcseq   : funcseq ',' decl    { $$ = adopt1 ($1, $3); }
          | /* empty */
          ;

block     : '{' stmtseq '}'     { $1->symbol = TOK_BLOCK;
                                  $$ = adopt1 ($1, $2); }
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
                                { $2->symbol = TOK_DECLID;
                                  $3->symbol = TOK_VARDECL;
                                  $1 = adopt2 ($1, $2, $4);
                                  $$ = adopt1 ($3, $1); }
          ;

while     : TOK_WHILE '(' expr ')' statement
                                { $$ = adopt2 ($1, $3, $5); }
          ;

ifelse    : TOK_IF '(' expr ')' statement
                                { $$ = adopt2 ($1, $3, $5); }
            TOK_IF '(' expr ')' statement TOK_ELSE statement
                                { $1->symbol = TOK_IFELSE;
                                  $1 = adopt2 ($1, $3, $5);
                                  $$ = adopt1 ($1, $7); }
          ;

return    : TOK_RETURN expr ';' { $$ = adopt1 ($1, $2); }
          | TOK_RETURN ';'      { $1->symbol = TOK_RETURNVOID;
                                  $$ = $1 }
          ;

expr      : binop                           { $$ = $1; }
          | unop                            { $$ = $1; }
          | allocator                       { $$ = $1; }
          | call                            { $$ = $1; }
          | '(' expr ')' %prec TOK_PAREN    { $$ = $2; }
          | variable                        { $$ = $1; }
          | constant                        { $$ = $1; }
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
          
unop      : '+' expr %prec TOK_POS  {$$ = adopt1sym ($1, $2, TOK_POS);}
          | '-' expr %prec TOK_NEG  {$$ = adopt1sym ($1, $2, TOK_NEG);}
          | '!' expr                {$$ = adopt1 ($1, $2);}
          | TOK_ORD expr            {$$ = adopt1 ($1, $2);}
          | TOK_CHR expr            {$$ = adopt1 ($1, $2);}
          ;

allocator : TOK_NEW IDENT '(' ')'
                            { $2->symbol = TOK_TYPEID;
                              $$ = adopt1 ($1, $2); }
          | TOK_NEW TOK_STRING '(' expr ')'
                            { $$ = adopt1sym ($1, $4, TOK_NEWSTRING); }
          | TOK_NEW basetype '[' expr ']'
                            { $1->symbol = TOK_NEWARRAY;
                              $$ = adopt2($1, $2, $4); }
          ;

call      : IDENT '(' ')' %prec TOK_CALL
                                    {$$ = adopt1sym($2, $1, TOK_CALL);}
          | IDENT '(' expr exprseq ')' %prec TOK_CALL
                                    { $2->symbol = TOK_CALL;
                                      $3 = adopt1($3, $4);
                                      $$ = adopt2($2, $1, $3); }
          ;

exprseq   : exprseq ',' expr        { $$ = adopt1($1, $3); }
          | /* empty */
          ;

variable  : IDENT                   { $$ = $1; }
          | expr '[' expr ']'
                                    { $2->symbol = TOK_INDEX;
                                      $$ = adopt2 ($2, $1, $3); }
          | expr '.' IDENT          { $3->symbol = TOK_FIELD;
                                      $$ = adopt2 ($2, $1, $3); }
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
