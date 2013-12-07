// Paul Scherer, pscherer@ucsc.edu

#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "auxlib.h"
#include "lyutils.h"
#include "oilprint.h"
#include "stringset.h"
#include "symtable.h"

string check_binop (astree* node, SymbolTable* types,
      SymbolTable* global);
string check_newarray (astree* node, SymbolTable* types,
      SymbolTable* global);
string check_expr (astree* node, SymbolTable* types,
      SymbolTable* global);
string check_statement (astree* node, SymbolTable* types,
      SymbolTable* global);

/*
 * Checks whether two types are equivalent and allowed with oc.
 * Returns "" if they don't match or not allowed and prints error
 * message.
 */
string are_compatible (string name1, string name2, size_t linenr) {
   if (strcmp (name1.c_str(), "bool") == 0 ||
         strcmp (name1.c_str(), "bool[]") == 0) {
      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
      }
      int comp = strcmp (name1.c_str(), name2.c_str());

      if (strcmp (name2.c_str(), "bool") == 0 ||
            strcmp (name2.c_str(), "bool[]") == 0) {
         return "bool";
      }
      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to bool\n", linenr);
      } else {
         return "bool";
      }
   }

   if (strcmp (name1.c_str(), "int") == 0 ||
         strcmp (name1.c_str(), "int[]") == 0) {
      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
      }

      int comp = strcmp (name1.c_str(), name2.c_str());

      if (strcmp (name2.c_str(), "int") == 0 ||
            strcmp (name2.c_str(), "int[]") == 0) {
         return "int";
      }
      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to int\n", linenr);
      } else {
         return "int";
      }
   }

   if (strcmp (name1.c_str(), "string") == 0 ||
         strcmp (name1.c_str(), "string[]") == 0) {

      if (strcmp (name2.c_str(), "null") == 0) {
         return "string";
      }

      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
         return "";
      }

      if (strcmp (name2.c_str(), "string") == 0 ||
            strcmp (name2.c_str(), "string[]") == 0) {
         return "string";
      }

      if (strcmp (name1.c_str(), "string[]") == 0 &&
            strcmp (name2.c_str(), "char") == 0) {
         return "char";
      }

      int comp = strcmp (name1.c_str(), name2.c_str());
      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to string (%s,%s)\n",
               linenr, name1.c_str(), name2.c_str());
      } else {
         return "string";
      }
   }

   if (strcmp (name1.c_str(), "char") == 0) {
      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name1.c_str(), name2.c_str());

      if (strcmp (name2.c_str(), "char") == 0 ||
            strcmp (name2.c_str(), "char[]") == 0) {
         return "char";
      }

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to char\n", linenr);
      } else {
         return "bool";
      }
   }

   if (strcmp (name1.c_str(), name2.c_str()) == 0) {
      return name1.c_str();
   }

   if (strcmp (name2.c_str(), "null") == 0) {
      return name1.c_str();
   }

   errprintf ("%zu: %s != %s\n", linenr, name1.c_str(), name2.c_str());
   set_exitstatus (EXIT_FAILURE);
   return "";
}

/*
 * Returns the constant type passed.
 */
string check_constant (astree* node) {
   astree* constant = node->children[0];
   string symbol = (char *)get_yytname (constant->symbol);

   if (strcmp (symbol.c_str(), "NUMBER") == 0)
      return "int";
   if (strcmp (symbol.c_str(), "TOK_STRCON") == 0)
      return "string";
   if (strcmp (symbol.c_str(), "TOK_CHARCON") == 0)
      return "char";
   if (strcmp (symbol.c_str(), "TOK_FALSE") == 0)
      return "bool";
   if (strcmp (symbol.c_str(), "TOK_TRUE") == 0)
      return "bool";
   if (strcmp (symbol.c_str(), "TOK_NULL") == 0)
      return "null";

   return "";
}

/*
 * Returns the variable type passed.
 */
string check_variable (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string name = "";
   string type = "";
   astree* ident_name = node->children[0];
   int field_cmp = strcmp (ident_name->lexinfo->c_str(), ".") == 0;
   int array_cmp = strcmp (ident_name->lexinfo->c_str(), "[") == 0;

   if (node->children.size() == 1 && (!field_cmp && !array_cmp)) {
      name = ident_name->lexinfo->c_str();
      return global->lookup(name, node->linenr);
   }

   if (field_cmp) {
      int field_index = 1;
      string fn_name = check_expr (ident_name->children[0], types,
            global);
      SymbolTable* getScope = types->lookup_param(fn_name,
            node->linenr);
      string fn_type = ident_name->children[field_index]->lexinfo->
            c_str();

      if (getScope != NULL)
         return getScope->lookup(fn_type, node->linenr);
   } else if (array_cmp) {
      int array_index = 1;
      astree* check_int = ident_name->children[array_index];

      if (strcmp (check_expr (check_int, types, global).c_str(),
            "int") != 0) {
         errprintf ("%zu: Must be [int]\n", node->linenr);
      } else {
         astree* fn = ident_name->children[0];
         string fn_type = check_expr (fn, types, global);
         if (strcmp (fn_type.c_str(), "string") == 0)
            return "char";
         return fn_type;
      }
   }

   return "";
}

/*
 * Returns the type of the function call that was passed.
 */
string check_call (astree* node, SymbolTable* global) {
   astree* ident = node->children[0];
   string ident_name = ident->lexinfo->c_str();

   vector<string> type = global->parseSignature(global->
         lookup(ident_name, node->linenr));
   return type.front();
}

string check_unop (astree* node, SymbolTable* types,
      SymbolTable* global) {
   astree* unop_check = node->children[0];

   if (strcmp ((char *)get_yytname (unop_check->symbol),
         "TOK_ORD") == 0) {
      string ord = check_expr (node->children[0]->children[0], types,
            global);

      if (strcmp (ord.c_str(), "int") != 0 &&
            strcmp (ord.c_str(), "char") != 0) {
         errprintf ("%zu: Must be [int]\n", node->linenr);
      } else
         return "int";
   } else if (strcmp ((char *)get_yytname (unop_check->symbol),
         "TOK_CHR") == 0) {
      string chr = check_expr (node->children[0]->children[0], types,
            global);

      if (strcmp (chr.c_str(), "char") != 0) {
         errprintf ("%zu: Must be [int]\n", node->linenr);
      } else
         return "char";
   }

   return check_expr (node->children[0]->children[0], types,
         global);
}

/*
 * Returns the allocated type that was passed.
 */
string check_allocator (astree* node, SymbolTable* global) {
   string vari_name = node->children[0]->lexinfo->c_str();
   string const_type = get_yytname (node->children[0]->symbol);

   if (strcmp (const_type.c_str(), "IDENT") == 0)
      return global->lookup(vari_name, node->linenr);
   else
      return vari_name;
}

/*
 * Returns the type of the expression passed.
 */
string check_expr (astree* node, SymbolTable* types,
      SymbolTable* global) {
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BINOP") == 0) {
      return check_binop(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_UNOP") == 0) {
      return check_unop(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_ALLOCATOR") == 0) {
      return check_allocator(node, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CALL") == 0) {
      return check_call(node, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARIABLE") == 0) {
      return check_variable(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CONSTANT") == 0) {
      return check_constant(node);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_NEWARRAY") == 0) {
      return check_newarray(node, types, global);
   }

   return "";
}

/*
 * Returns the binary operation type passed.
 */
string check_binop (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string expr1 = check_expr (node->children[0], types, global);
   string expr2 = check_expr (node->children[2], types, global);
   return are_compatible(expr1, expr2, node->linenr);
}

/*
 * Returns the basetype of the node passed.
 */
string check_basetype (astree* node, SymbolTable* global) {
   astree* constant = node->children[0];
   string symbol = (char *)get_yytname (constant->symbol);

   if (strcmp (symbol.c_str(), "TOK_VOID") == 0)
      return "void";
   if (strcmp (symbol.c_str(), "TOK_BOOL") == 0)
      return "bool";
   if (strcmp (symbol.c_str(), "TOK_CHAR") == 0)
      return "char";
   if (strcmp (symbol.c_str(), "TOK_INT") == 0)
      return "int";
   if (strcmp (symbol.c_str(), "TOK_STRING") == 0)
      return "string";
   if (strcmp (symbol.c_str(), "TOK_IDENT") == 0) {
      string type = constant->lexinfo->c_str();
      return global->lookup(type, node->linenr);
   }

   return "";
}

/*
 * Returns the type of array that was initialized.
 */
string check_newarray (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string array_size = check_expr (node->children[1], types, global);

   if (strcmp (array_size.c_str(), "int") != 0) {
      errprintf ("%zu: Must be [int]\n", node->linenr);
   }

   string array_type = check_basetype (node->children[0], global);
   array_type.append ("[]");
   return array_type;
}

/*
 * Returns the return type passed.
 */
string check_return (astree* node, SymbolTable* types,
      SymbolTable* global) {
   if (node->children.size() == 0) {
      return "void";
   } else {
      return check_expr (node->children[0], types, global);
   }
}

/*
 * Returns the type of the comparision statement for while loop passed.
 */
string check_while (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string compare = check_expr (node->children[0], types, global);
   //global = global->enter_block (node->linenr);

   if (strcmp (compare.c_str(), "bool") != 0) {
      errprintf ("%zu: Must be (bool)\n", node->linenr);
      return "";
   }

   return check_statement (node->children[1], types, global);
}

/*
 * Returns the type of the vardecl statement passed.
 */
string check_vardecl (astree* node, SymbolTable* types,
      SymbolTable* global) {
   int name_index = 1;
   string name = node->children[name_index]->lexinfo->c_str();
   string type = global->lookup(name, node->linenr);

   string expr = check_expr (node->children[2], types, global);

   //fprintf (stderr, "%s:%s\n", type.c_str(), expr.c_str());
   return are_compatible (type, expr, node->linenr);
}

/*
 * Returns the type of the if else statement passed.
 */
string check_ifelse (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string compare = check_expr (node->children[0], types, global);

   if (strcmp (compare.c_str(), "bool") != 0) {
      errprintf ("%zu: Must be (bool)\n", node->linenr);
      return "";
   }

   check_statement (node->children[1], types, global);

   if (node->children.size() > 2) {

      check_statement (node->children[2], types, global);

      if (global->getParent() != NULL) {
         global = global->getParent();
      }
   }

   return "bool";
}

/*
 * Returns the type of the statement passed.
 */
string check_statement (astree* node, SymbolTable* types,
      SymbolTable* global) {
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARDECL") == 0) {
      return check_vardecl(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_WHILE") == 0) {
      return check_while(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_IFELSE") == 0 ||
         strcmp ((char *)get_yytname (node->symbol),
               "TOK_IF") == 0) {
      return check_ifelse(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_RETURN") == 0) {
      return check_return(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_EXPR") == 0) {
      return check_expr(node, types, global);
   }

   return "";
}

/*
 * Returns the type of the block passed.
 */
string check_block (astree* node, SymbolTable* types,
      SymbolTable* global) {
   if (strcmp ((char *)get_yytname (node->children[0]->symbol),
         "TOK_BLOCK") == 0) {
      return check_block (node->children[0], types, global);
   } else {
      return check_statement (node->children[0], types, global);
   }
}

/*
 * Traverses through the AST, performing typechecks for each line
 * of the oc code.
 */
void typecheck_rec (astree* node, SymbolTable* types,
      SymbolTable* global, int depth) {
   if (node == NULL) return;

   for (size_t child = 0; child < node->children.size();
         ++child) {
      SymbolTable* childBlock;
      if (strcmp ((char *)get_yytname (node->symbol),
            "TOK_FUNCTION") == 0) {
         int ident_index = 1;

         astree* block_node = node->children[2];
         if (strcmp ((char *)get_yytname (block_node->symbol),
               "TOK_PARAMETER") == 0) {
            block_node = node->children[3];
         }

         string fn_name = node->children[ident_index]->lexinfo->c_str();
         global = global->lookup_param(fn_name, node->linenr);
      } else if (strcmp ((char *)get_yytname (node->symbol),
            "TOK_IF") == 0 ||
            strcmp ((char *)get_yytname (node->symbol),
                  "TOK_WHILE") == 0 ||
                  strcmp ((char *)get_yytname (node->symbol),
                        "TOK_IFELSE") == 0) {
         childBlock = global->enter_block (node->blockNum);
         if (childBlock != NULL)
            global = childBlock;
      }

      typecheck_rec (node->children[child], types, global, depth + 1);

      if (childBlock != NULL &&
            (strcmp ((char *)get_yytname (node->symbol),
            "TOK_FUNCTION") == 0 ||
            strcmp ((char *)get_yytname (node->symbol),
                  "TOK_WHILE") == 0)) {
         global = global->getParent();
      }
   }

   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARDECL") == 0) {
      check_vardecl (node, types, global);
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BINOP") == 0) {
      string expr1 = check_expr (node->children[0], types, global);
      string expr2 = check_expr (node->children[2], types, global);

      are_compatible (expr1, expr2, node->linenr);
   }
}
