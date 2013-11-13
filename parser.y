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

%token ROOT IDENT NUMBER TOK_FUNCTION TOK_PROTOTYPE TOK_BASETYPE
%token TOK_TYPE TOK_CONSTANT TOK_VARIABLE TOK_UNOP TOK_BINOP
%token TOK_ALLOCATOR

%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_INTCON TOK_CHARCON TOK_STRCON TOK_NEWSTRING

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_DECLID TOK_RETURNVOID
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_INDEX TOK_VARDECL TOK_PARAMLIST

%right TOK_THEN TOK_ELSE
%right '='
%left  TOK_EQ TOK_NE TOK_LE TOK_GE TOK_LT TOK_GT
%left  '+' '-'
%left  '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_ORD TOK_CHR
%left  TOK_CALL
%left  '[' '.'
%nonassoc TOK_NEW
%nonassoc TOK_PAREN

%start start

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
        
structdef : TOK_STRUCT IDENT declseq '}'
                                { $2->symbol = TOK_TYPEID;
                                  $$ = adopt2 ($1, $2, $3); }
          | TOK_STRUCT IDENT '{' '}'
                                { $2->symbol = TOK_TYPEID;
                                  $$ = adopt1 ($1, $2); }
          ;

declseq   : '{' decl ';'        { $$ = $2; }
          | declseq decl ';'    { $$ = adopt1 ($1, $2); }
          ;

decl      : type IDENT          { $2->symbol = TOK_DECLID;
                                  $$ = adopt1 ($1, $2); }
          ;

type      : basetype TOK_ARRAY  { astree* temp = new_astree (TOK_TYPE,
                                  $1->filenr, $1->linenr, $1->offset,
                                  "type");
                                  $2 = adopt1 ($2, $1);
                                  $$ = adopt1 (temp, $2); }
          | basetype            { astree* temp = new_astree (TOK_TYPE,
                                  $1->filenr, $1->linenr, $1->offset,
                                  "type");
                                  $$ = adopt1 (temp, $1); }
          ; 

basetype  : TOK_VOID            { astree* temp = new_astree
                                  (TOK_BASETYPE, $1->filenr,
                                  $1->linenr, $1->offset, "basetype");
                                  $$ = adopt1 (temp, $1); }
          | TOK_BOOL            { astree* temp = new_astree
                                  (TOK_BASETYPE, $1->filenr,
                                  $1->linenr, $1->offset, "basetype");
                                  $$ = adopt1 (temp, $1); }
          | TOK_CHAR            { astree* temp = new_astree
                                  (TOK_BASETYPE, $1->filenr,
                                  $1->linenr, $1->offset, "basetype");
                                  $$ = adopt1 (temp, $1); }
          | TOK_INT             { astree* temp = new_astree
                                  (TOK_BASETYPE, $1->filenr,
                                  $1->linenr, $1->offset, "basetype");
                                  $$ = adopt1 (temp, $1); }
          | TOK_STRING          { astree* temp = new_astree
                                  (TOK_BASETYPE, $1->filenr,
                                  $1->linenr, $1->offset, "basetype");
                                  $$ = adopt1 (temp, $1); }
          | IDENT               { astree* temp = new_astree
                                  (TOK_BASETYPE, $1->filenr,
                                  $1->linenr, $1->offset, "basetype");
                                  $$ = adopt1 (temp, $1); }
          ;
          
function  : type IDENT funcseq ')' block
                              { astree* temp = new_astree (TOK_FUNCTION,
                                $1->filenr, $1->linenr, $1->offset,
                                "function");
                                $2->symbol = TOK_DECLID;
                                $1 = adopt2 ($1, $2, $3);
                                $1 = adopt1 ($1, $5);
                                $$ = adopt1 (temp, $1); }
          | type IDENT '(' ')' block
                              { astree* temp = new_astree (TOK_FUNCTION,
                                $1->filenr, $1->linenr, $1->offset,
                                "function");
                                $2->symbol = TOK_DECLID;
                                $1 = adopt2 ($1, $2, $5);
                                $$ = adopt1 (temp, $1); }
          ;

prototype : type IDENT funcseq ')' ';'
                            { astree* temp = new_astree (TOK_PROTOTYPE,
                              $1->filenr, $1->linenr, $1->offset,
                              "prototype");
                              $2->symbol = TOK_DECLID;
                              $1 = adopt2 ($1, $2, $3);
                              $$ = adopt1 (temp, $1); }
          | type IDENT '(' ')' ';'
                            { astree* temp = new_astree (TOK_PROTOTYPE,
                              $1->filenr, $1->linenr, $1->offset,
                              "prototype");
                              $2->symbol = TOK_DECLID;
                              $1 = adopt1 ($1, $2);
                              $$ = adopt1 (temp, $1); }
          ;

funcseq   : '(' decl            { $1->symbol = TOK_PARAMLIST;
                                  $$ = adopt1 ($1, $2); }
          | funcseq ',' decl    { $$ = adopt1 ($1, $3); }
          ;

block     : stmtseq '}'         { $$ = $1; }
          | '{' '}'             { $1->symbol = TOK_BLOCK;
                                  $$ = $1; }
          ;

stmtseq   : '{' statement       { $1->symbol = TOK_BLOCK;
                                  $$ = adopt1 ($1, $2); }
          | stmtseq statement   { $$ = adopt1 ($1, $2); }
          ;

statement : block               { $$ = $1; }
          | vardecl             { $$ = $1; }
          | while               { $$ = $1; }
          | ifelse              { $$ = $1; }
          | return              { $$ = $1; }
          | expr ';'            { $$ = $1; }
          | ';'                 { $1->symbol = TOK_BLOCK;
                                  $$ = $1;}
          ;

vardecl   : type IDENT '=' expr ';'
                                { $2->symbol = TOK_DECLID;
                                  $3->symbol = TOK_VARDECL;
                                  $3 = adopt2 ($3, $1, $2);
                                  $$ = adopt1 ($3, $4); }
          ;

while     : TOK_WHILE '(' expr ')' statement
                                { $$ = adopt2 ($1, $3, $5); }
          ;

ifelse    : TOK_IF '(' expr ')' statement %prec TOK_THEN
                                { $$ = adopt2 ($1, $3, $5); }
          | TOK_IF '(' expr ')' statement TOK_ELSE statement
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

binop     : expr '=' expr          { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr TOK_EQ expr       { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr TOK_NE expr       { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr TOK_LE expr       { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr TOK_GE expr       { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr TOK_LT expr       { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr TOK_GT expr       { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr '+' expr          { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr '-' expr          { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr '*' expr          { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr '/' expr          { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          | expr '%' expr          { astree* temp = new_astree
                                     (TOK_BINOP, $1->filenr, $1->linenr,
                                     $1->offset, "binop");
                                     temp = adopt2 (temp, $1, $2);
                                     $$ = adopt1 (temp, $3); }
          ;
          
unop      : '+' expr %prec TOK_POS  { astree* temp = new_astree
                                     (TOK_UNOP, $1->filenr, $1->linenr,
                                     $1->offset, "unop");
                                     $1 = adopt1sym ($1, $2, TOK_POS);
                                     $$ = adopt1 (temp, $1); }
          | '-' expr %prec TOK_NEG  { astree* temp = new_astree
                                      (TOK_UNOP, $1->filenr, $1->linenr,
                                      $1->offset, "unop");
                                      $1 = adopt1sym ($1, $2, TOK_NEG);
                                      $$ = adopt1 (temp, $1); }
          | '!' expr                { astree* temp = new_astree
                                      (TOK_UNOP, $1->filenr, $1->linenr,
                                      $1->offset, "unop");
                                      $1 = adopt1 ($1, $2);
                                      $$ = adopt1 (temp, $1); }
          | TOK_ORD expr            { astree* temp = new_astree
                                      (TOK_UNOP, $1->filenr, $1->linenr,
                                      $1->offset, "unop");
                                      $1 = adopt1 ($1, $2);
                                      $$ = adopt1 (temp, $1); }
          | TOK_CHR expr            { astree* temp = new_astree
                                      (TOK_UNOP, $1->filenr, $1->linenr,
                                      $1->offset, "unop");
                                      $1 = adopt1 ($1, $2);
                                      $$ = adopt1 (temp, $1); }
          ;

allocator : TOK_NEW TOK_STRING '(' expr ')'
                            { astree* temp = new_astree (TOK_ALLOCATOR,
                              $1->filenr, $1->linenr, $1->offset,
                              "allocator");
                              $1 = adopt1sym ($1, $4, TOK_NEWSTRING);
                              $$ = adopt1 (temp, $1); }
          | TOK_NEW IDENT '(' ')'
                            { astree* temp = new_astree (TOK_ALLOCATOR,
                              $1->filenr, $1->linenr, $1->offset,
                              "allocator");
                              $2->symbol = TOK_TYPEID;
                              $1 = adopt1 ($1, $2);
                              $$ = adopt1 (temp, $1); }
          | TOK_NEW basetype '[' expr ']'
                            { astree* temp = new_astree (TOK_ALLOCATOR,
                              $1->filenr, $1->linenr, $1->offset,
                              "allocator");
                              $1->symbol = TOK_NEWARRAY;
                              $1 = adopt2 ($1, $2, $4);
                              $$ = adopt1 (temp, $1); }
          ;

call      : IDENT '(' exprseq ')' %prec TOK_CALL
                                { $2->symbol = TOK_CALL;
                                   $$ = adopt2($2, $1, $3); }
          | IDENT '(' ')' %prec TOK_CALL
                                { $$ = adopt1sym($2, $1, TOK_CALL); }
          ;

exprseq   : expr                { $$ = $1; }
          | exprseq ',' expr    { $$ = adopt1 ($1, $3); }
          ;

variable  : IDENT               { astree* temp = new_astree
                                   (TOK_VARIABLE, $1->filenr,
                                   $1->linenr, $1->offset, "variable");
                                   $$ = adopt1 (temp, $1); }
          | expr '[' expr ']'   { astree* temp = new_astree
                                   (TOK_VARIABLE, $1->filenr,
                                   $1->linenr, $1->offset, "variable");
                                   $2->symbol = TOK_INDEX;
                                   $2 = adopt2 ($2, $1, $3);
                                   $$ = adopt1 (temp, $2); }
          | expr '.' IDENT      { astree* temp = new_astree
                                   (TOK_VARIABLE, $1->filenr,
                                   $1->linenr, $1->offset, "variable");
                                   $3->symbol = TOK_FIELD;
                                   $2 = adopt2 ($2, $1, $3);
                                   $$ = adopt1 (temp, $2); }
          ;

constant  : NUMBER              { astree* temp = new_astree
                                  (TOK_CONSTANT, $1->filenr,
                                  $1->linenr, $1->offset, "constant");
                                  $$ = adopt1 (temp, $1); }
          | TOK_STRCON          { astree* temp = new_astree
                                  (TOK_CONSTANT, $1->filenr, $1->linenr,
                                  $1->offset, "constant");
                                  $$ = adopt1 (temp, $1); }
          | TOK_CHARCON         { astree* temp = new_astree
                                  (TOK_CONSTANT, $1->filenr, $1->linenr,
                                  $1->offset, "constant");
                                  $$ = adopt1 (temp, $1); }
          | TOK_FALSE           { astree* temp = new_astree
                                  (TOK_CONSTANT, $1->filenr, $1->linenr,
                                  $1->offset, "constant");
                                  $$ = adopt1 (temp, $1); }
          | TOK_TRUE            { astree* temp = new_astree
                                  (TOK_CONSTANT, $1->filenr, $1->linenr,
                                  $1->offset, "constant");
                                  $$ = adopt1 (temp, $1); }
          | TOK_NULL            { astree* temp = new_astree
                                  (TOK_CONSTANT, $1->filenr, $1->linenr,
                                  $1->offset, "constant");
                                  $$ = adopt1 (temp, $1); }
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
