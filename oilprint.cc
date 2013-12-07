// Paul Scherer, pscherer@ucsc.edu

#include <assert.h>
#include <errno.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "astree.h"
#include "auxlib.h"
#include "lyutils.h"
#include "symtable.h"
#include "typecheck.h"

const int INDENT = 8;
// Maps name to pointer
map<string,string> struct_map;
int blocknr = 1;
int b_counter = 1;
int i_counter = 1;
int p_counter = 1;
int s_counter = 1;
int ifelse_counter = 1;
int while_counter = 1;
int stmt_label = 1;

enum Category { GLOBAL, LOCAL, STRUCT, FIELD };

string oil_expr (FILE* outfile, astree* node, SymbolTable* types,
      SymbolTable* global, int category, int depth);

/*
 * Returns correct register category for passed datatype.
 */
string reg_category (string type) {
   // Convert the variable counter to a string
   std::ostringstream ostr;
   string var_count = ostr.str();
   if (strcmp (type.c_str(), "int") == 0) {
      ostr << i_counter;
      string reg = "i" + ostr.str();
      i_counter++;
      return reg;
   }

   if (strcmp (type.c_str(), "ubyte") == 0) {
      ostr << b_counter;
      string reg = "b" + ostr.str();
      b_counter++;
      return reg;
   }

   if (type.find ("*") != std::string::npos) {
      ostr << p_counter;
      string reg = "p" + ostr.str();
      p_counter++;
      return reg;
   }

   ostr << s_counter;
   string reg = "s" + ostr.str();
   s_counter++;
   return reg;
}

string convert_ident (string name, string field_name, int category) {
   std::ostringstream ostr;
   ostr << blocknr;
   string blocknumber = ostr.str();
   string new_ident = "";
   switch (category) {
   case GLOBAL:
      return ("__" + name).c_str();
   case LOCAL:
      new_ident = "_" + blocknumber;
      new_ident.append("_");
      new_ident.append(name);
      return new_ident;
   case STRUCT:
      return ("s_" + name).c_str();
   case FIELD:
      return ("f_" + name + "_" + field_name).c_str();
   }

   return NULL;
}

/*
 * Converts the type to oil program type.
 */
string converted_type (string type) {
   if (strcmp (type.c_str(), "bool") == 0 ||
         strcmp (type.c_str(), "char") == 0) {
      return "ubyte ";
   }

   if (strcmp (type.c_str(), "int") == 0) {
      return "int ";
   }

   if (strcmp (type.c_str(), "string") == 0 ||
         strcmp (type.c_str(), "bool[]") == 0 ||
         strcmp (type.c_str(), "char[]") == 0) {
      return "ubyte *";
   }

   if (strcmp (type.c_str(), "int[]") == 0) {
      return "int *";
   }

   if (strcmp (type.c_str(), "string[]") == 0) {
      return "ubyte **";
   }

   size_t pos = type.find("[]");
   if (pos != std::string::npos) {
      string type_id = type.substr(0, pos);
      string converted = "struct " + type_id + " **";
      return converted;
   }

   return type + " ";
}

string convert_expr (string expr, SymbolTable* global) {
   if (strcmp (global->lookup_oil(expr).c_str(), "") != 0) {
      if (global->is_global (expr)) {
         return convert_ident (expr, "", GLOBAL);
      } else {
         return convert_ident (expr, "", LOCAL);
      }
   }

   return expr;
}

/*
 * Prints struct declarations to outfile.
 */
void print_param (FILE* outfile, SymbolTable* types) {
   std::map<string,string>::iterator it;
   map<string,string> mapping = types->getMapping();

   for (it = mapping.begin(); it != mapping.end(); ++it) {
      string name = it->first;
      string type = it->second;

      fprintf (outfile, "%*s%s %s;\n", INDENT, "",
            converted_type (type).c_str(), name.c_str());
   }
}

bool has_register_name (string ident) {
   if (ident.size() == 2 &&
         (ident.at(0) == 'i' ||
               ident.at(0) == 'p' ||
               ident.at(0) == 's' ||
               ident.at(0) == 'b')) {
      return true;
   }

   return false;
}

/*
 * Returns the lexical constant passed.
 */
string oil_constant (astree* node) {
   astree* constant = node->children[0];
   string symbol = (char *)get_yytname (constant->symbol);

   if (strcmp (symbol.c_str(), "TOK_FALSE") == 0)
      return "0";
   if (strcmp (symbol.c_str(), "TOK_TRUE") == 0)
      return "1";
   if (strcmp (symbol.c_str(), "TOK_NULL") == 0)
      return "0";

   return constant->lexinfo->c_str();
}

/*
 * Returns the converted variable name passed.
 */
string oil_variable (FILE* outfile, astree* node, SymbolTable* types,
      SymbolTable* global, int category, int depth) {
   string name = "";
   string type = "";
   astree* ident_name = node->children[0];
   int field_cmp = strcmp (ident_name->lexinfo->c_str(), ".") == 0;
   int array_cmp = strcmp (ident_name->lexinfo->c_str(), "[") == 0;

   if (node->children.size() == 1 && (!field_cmp && !array_cmp)) {
      name = ident_name->lexinfo->c_str();
      return name;
   }

   if (field_cmp) {
      int field_index = 1;
      string fn_name = oil_variable (outfile, ident_name->children[0],
            types, global, category, depth);
      string fn_type = ident_name->children[field_index]->lexinfo->
            c_str();

      return convert_ident (fn_name, "", category) + "." + fn_type;
   } else if (array_cmp) {
      astree* fn = ident_name->children[0];
      return convert_ident (fn->lexinfo->c_str(), "", category);
   }

   return NULL;
}

/*
 * Returns the binary operation passed.
 */
string oil_binop (FILE* outfile, astree* node, SymbolTable* types,
      SymbolTable* global, int category, int depth) {
   string binop = node->children[1]->lexinfo->c_str();
   string expr1 = oil_expr (outfile, node->children[0], types, global,
         category, depth);
   string expr2 = oil_expr (outfile, node->children[2], types, global,
         category, depth);

   int is_conditional = strcmp (binop.c_str(), "<") == 0 ||
         strcmp (binop.c_str(), "<=") == 0 ||
         strcmp (binop.c_str(), ">") == 0 ||
         strcmp (binop.c_str(), ">=") == 0 ||
         strcmp (binop.c_str(), "==") == 0 ||
         strcmp (binop.c_str(), "!=") == 0;

   expr1 = convert_expr (expr1, global);
   expr2 = convert_expr (expr2, global);

   if (is_conditional) {
      string type = converted_type ("bool");
      string register_cat = reg_category (type);
      fprintf (outfile, "%*s%s%s = %s %s %s;\n", depth * INDENT, "",
            type.c_str(), register_cat.c_str(), expr1.c_str(),
            binop.c_str(), expr2.c_str());

      return register_cat;
   } else {
      string type = check_expr (node->children[2], types, global);
      string register_cat = reg_category (type);
      type = converted_type (type);
      fprintf (outfile, "%*s%s%s = %s %s %s;\n", depth * INDENT, "",
            type.c_str(), register_cat.c_str(), expr1.c_str(),
            binop.c_str(), expr2.c_str());

      return register_cat;
   }
}

/*
 * Returns the converted name of the function call passed.
 */
string oil_call (astree* node) {
   astree* ident = node->children[0];
   string ident_name = ident->lexinfo->c_str();

   return convert_ident (ident_name, "", GLOBAL);
}

string oil_unop (FILE* outfile, astree* node, SymbolTable* types,
      SymbolTable* global, int category, int depth) {
   astree* unop_check = node->children[0];

   if (strcmp ((char *)get_yytname (unop_check->symbol),
         "TOK_ORD") == 0) {
      string ord = oil_expr (outfile, node->children[0]->children[0],
            types, global, category, depth);

      return "(int)" + ord;
   } else if (strcmp ((char *)get_yytname (unop_check->symbol),
         "TOK_CHR") == 0) {
      string chr = oil_expr (outfile, node->children[0]->children[0],
            types, global, category, depth);

      return "ubyte" + chr;
   }

   string unop = "(";
   unop.append (unop_check->lexinfo->c_str());
   unop.append (oil_expr (outfile, node->children[0]->children[0],
         types, global, category, depth));
   return unop + ")";
}

/*
 * Returns the name of the expression passed.
 */
string oil_expr (FILE* outfile, astree* node, SymbolTable* types,
      SymbolTable* global, int category, int depth) {
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BINOP") == 0) {
      return oil_binop(outfile, node, types, global, category, depth);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_UNOP") == 0) {
      return oil_unop(outfile, node, types, global, category, depth);
   }/*
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_ALLOCATOR") == 0) {
      return oil_allocator(node, global, category);
   }*/
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CALL") == 0) {
      return oil_call(node);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARIABLE") == 0) {
      return oil_variable(outfile, node, types, global,
            category, depth);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CONSTANT") == 0) {
      return oil_constant(node);
   }
   /*if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_NEWARRAY") == 0) {
      return oil_newarray(node, types, global);
   }*/

   return "";
}

void traverse_oil (FILE* outfile, astree* root, SymbolTable* types,
      SymbolTable* global, int depth, int category) {

   int cmp_vardecl = strcmp ((char *)get_yytname (root->symbol),
         "TOK_VARDECL") == 0;
   int cmp_binop = strcmp ((char *)get_yytname (root->symbol),
         "TOK_BINOP") == 0;
   int cmp_call = strcmp ((char *)get_yytname (root->symbol),
         "TOK_CALL") == 0;
   int cmp_block = strcmp ((char *)get_yytname (root->symbol),
         "TOK_BLOCK") == 0;
   int cmp_while = strcmp ((char *)get_yytname (root->symbol),
         "TOK_WHILE") == 0;
   /*int cmp_if = strcmp ((char *)get_yytname (root->symbol),
            "TOK_IF") == 0;
   int cmp_ifelse = strcmp ((char *)get_yytname (root->symbol),
         "TOK_IFELSE") == 0;*/

   if (cmp_vardecl) {
      astree* alloc = root->children[2];
      int cmp_alloc = strcmp ((char *)get_yytname (alloc->symbol),
            "TOK_ALLOCATOR") == 0;
      int cmp_newarray = strcmp ((char *)get_yytname (alloc->symbol),
            "TOK_NEWARRAY") == 0;
      int name_index = 1;

      string name = root->children[name_index]->lexinfo->c_str();
      string type = global->lookup_oil(name);

      string expr = oil_expr (outfile, root->children[2], types, global,
            category, depth);
      string converted_name = convert_ident (name, "", category);

      if (cmp_alloc) {
         string reg_cat = reg_category ("*");

         fprintf (outfile,
               "%*sstruct %s *%s = xcalloc (1, sizeof (struct %s));\n",
               depth * INDENT, "", type.c_str(),
               reg_cat.c_str(), type.c_str());
         fprintf (outfile, "%*s%s = *%s;\n", depth * INDENT, "",
               converted_name.c_str(), reg_cat.c_str());

         // Map the variable name to the created struct pointer
         struct_map[name] = reg_cat;
      } else if (cmp_newarray) {
         if (alloc->children.size() == 2) {
            string reg_cat = reg_category (type);
            string operand = oil_expr (outfile, alloc->children[1],
                  types, global, category, depth);
            string con_type = converted_type (type);
            fprintf (outfile,
                  "%*s%s%s = xcalloc (%s, sizeof (%s));\n",
                  depth * INDENT, "", con_type.c_str(),
                  reg_cat.c_str(), operand.c_str(), con_type.c_str());
         } else {

         }
      } else if (depth == 1 && category == GLOBAL) {
         fprintf (outfile, "%*s%s = %s;\n", depth * INDENT, "",
               converted_name.c_str(), expr.c_str());
      } else {
         fprintf (outfile, "%*s%s%s = %s;\n", depth * INDENT, "",
               converted_type (type).c_str(),
               converted_name.c_str(), expr.c_str());
      }
   } else if (cmp_block) {
      astree* check_child = root->children[0];
      if (strcmp ((char *)get_yytname (check_child->symbol),
            "TOK_IF") == 0) {
         string expr = oil_expr (outfile, check_child->children[0],
               types, global, category, depth);

         expr = convert_expr(expr, global);
         fprintf (outfile, "%*sif (!%s) goto fi_%d;\n",
               (depth - 1) * INDENT, "", expr.c_str(), ifelse_counter);

         // Traverse through statements within if block
         for (size_t child = 1; child < check_child->children.size();
               ++child) {
            traverse_oil (outfile, check_child->children[child], types,
                  global, depth + 1, LOCAL);
         }

         fprintf (outfile, "%*sfi_%d:;\n",
               (depth - 1) * INDENT, "", ifelse_counter++);
      }
   } else if (cmp_while) {
      fprintf (outfile, "%*swhile_%d:;\n",
            (depth - 1) * INDENT, "", while_counter++);

      string expr = oil_expr (outfile, root->children[0],
            types, global, category, depth);
      expr = convert_expr (expr, global);

      fprintf (outfile, "%*sif (!%s) goto break_%d;\n",
            depth * INDENT, "", expr.c_str(), while_counter);
      // Traverse through statements within while block
      astree* block = root->children[1];
      for (size_t child = 0; child < block->children.size();
            ++child) {
         traverse_oil (outfile, block->children[child], types,
               global, depth + 1, LOCAL);
      }

      fprintf (outfile, "%*sgoto while_%d;\n",
            (depth + 1) * INDENT, "", while_counter - 1);
      fprintf (outfile, "%*sbreak_%d:;\n",
            (depth - 1) * INDENT, "", while_counter);
   }else if (cmp_binop) {
      astree* binop_sym = root->children[1];
      astree* expr1node = root->children[0];
      astree* expr2node = root->children[2];

      string binop = binop_sym->lexinfo->c_str();
      string expr1 = oil_expr (outfile, expr1node, types, global,
            category, depth);
      string expr2 = oil_expr (outfile, expr2node, types, global,
            category, depth);

      expr1 = convert_expr(expr1, global);
      expr2 = convert_expr(expr2, global);

      // If +,-,*,/ then assign this to a temporary variable
      if (strcmp (binop_sym->lexinfo->c_str(), "=") == 0) {
         fprintf (outfile, "%*s%s = %s;\n", depth * INDENT, "",
               expr1.c_str(), expr2.c_str());
      } else {
         /*string type = check_expr (expr2node, types, global);
         string register_cat = reg_category (type);
         type = converted_type (type);

         fprintf (outfile, "%*s%s%s = %s %s %s;\n", depth * INDENT, "",
               type.c_str(), register_cat.c_str(), expr1.c_str(),
               binop.c_str(), expr2.c_str());*/
      }
   } else if (cmp_call) {
      string fn_call = oil_call (root);

      fprintf (outfile, "%*s%s (", depth * INDENT, "",
            fn_call.c_str());
      // If the call has parameters
      if (root->children.size() >= 2) {
         string expr = oil_expr (outfile, root->children[1],
               types, global, category, depth);

         expr = convert_expr(expr, global);
         fprintf (outfile, "%s", expr.c_str());

         // Check if there are more
         astree* expr_seq = root->children[1];
         if (expr_seq->children.size() > 1) {
            fprintf (outfile, ", ");

            for (size_t size = 1; size < expr_seq->children.size();
                  size++) {
               string expr = oil_expr (outfile,
                     expr_seq->children[size], types, global, category,
                     depth);

               expr = convert_expr(expr, global);
               fprintf (outfile, "%s", expr.c_str());

               if (size < expr_seq->children.size() - 1) {
                  fprintf (outfile, ", ");
               }
            }
         }
      }

      fprintf (outfile, ");\n");
   }

   /*if (cmp_while || cmp_if || cmp_ifelse)
      global = global->getParent();*/
}

void traverse_ast (FILE* outfile, astree* root, SymbolTable* types,
      SymbolTable* global, int depth, int category) {
   for (size_t child = 0; child < root->children.size();
         ++child) {
      traverse_oil (outfile, root->children[child], types, global,
            depth, category);
   }
}
void generate_oil_func (FILE* outfile, astree* root, SymbolTable* types,
      SymbolTable* global, int depth) {
   if (root == NULL) return;

   for (size_t child = 0; child < root->children.size();
         ++child) {
      generate_oil_func (outfile, root->children[child], types,
            global, depth + 1);
   }

   if (strcmp ((char *)get_yytname (root->symbol),
         "TOK_FUNCTION") == 0) {
      int name_index = 1;
      int block_index = 2;

      string func_name = root->children[name_index]->lexinfo->
            c_str();
      string func_type = global->lookup(func_name, root->linenr);

      vector<string> signature = global->parseSignature(func_type);

      fprintf (outfile, "%s\n__%s(\n", signature[0].c_str(),
            func_name.c_str());

      // If there are parameters for the function
      if (signature.size() > 1) {
         block_index = 3;
         global = global->lookup_param (func_name,
               root->linenr);
         map<string,string> mapping = global->getMapping();
         astree* param_type = root->children[2];

         for (size_t size = 1; size < signature.size(); ++size) {
            astree* declid = param_type->children[size - 1]->
                  children[1];
            fprintf (outfile, "%*s%s%s", INDENT, "",
                  converted_type (signature[size]).c_str(),
                  convert_ident (declid->lexinfo->c_str(), "", LOCAL).
                  c_str());

            // If last parameter
            if (size + 1 == signature.size()) {
               fprintf (outfile, ")\n");
            } else {
               fprintf (outfile, ",\n");
            }
         }
      } else {
         fprintf (outfile, ")\n");
      }

      fprintf (outfile, "{\n");
      traverse_oil (outfile, root->children[block_index], types,
            global, 1, LOCAL);
      fprintf (outfile, "}\n");

      if (global->getParent() != NULL)
         global = global->getParent();
   }
}

bool is_struct (string name, SymbolTable* types) {
   if (strcmp (types->lookup_oil(name).c_str(), "") != 0) {
      return true;
   }

   return false;
}

void generate_oil (FILE* outfile, astree* root, SymbolTable* types,
      SymbolTable* global) {
   fprintf (outfile, "#define __OCLIB_C__\n"
         "#include \"oclib.oh\"\n");

   // Print structs, if any
   if (!types->getMapping().empty()) {
      std::map<string,string>::iterator it;
      map<string,string> mapping = types->getMapping();

      for (it = mapping.begin(); it != mapping.end(); ++it) {
         string name = it->first;

         fprintf (outfile, "\nstruct %s {\n", name.c_str());

         SymbolTable* getScope = types->lookup_param_oil(name);

         print_param (outfile, getScope);

         fprintf (outfile, "};\n");
      }
   }

   // Print global variable declarations, if any
   if (!global->getMapping().empty()) {
      std::map<string,string>::iterator it;
      map<string,string> mapping = global->getMapping();

      for (it = mapping.begin(); it != mapping.end(); ++it) {
         if (it->second.find("(") == std::string::npos) {
            if (is_struct (it->second, types)) {
               fprintf (outfile, "\nstruct %s%s;",
                     converted_type (it->second).c_str(),
                     convert_ident (it->first, "",  GLOBAL).c_str());
            } else {
               fprintf (outfile, "\n%s%s;",
                     converted_type (it->second).c_str(),
                     convert_ident (it->first, "",  GLOBAL).c_str());
            }
         }
      }

      fprintf (outfile, "\n");
   }

   // Print all functions with parameters and statements, if any
   generate_oil_func (outfile, root, types, global, 0);

   // Print the global statements
   fprintf (outfile, "\nvoid __ocmain ()\n{\n");
   traverse_ast (outfile, root, types, global, 1, GLOBAL);
   fprintf (outfile, "}\n");

}
