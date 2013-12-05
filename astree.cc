// Paul Scherer, pscherer@ucsc.edu

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "stringset.h"
#include "lyutils.h"

static const size_t FN_WITH_PARAM = 4;
static const size_t STRUCT_WITH_PARAM = 2;
static const size_t PROTO_WITH_PARAM = 3;

astree* new_astree (int symbol, int filenr, int linenr,
      int offset, const char* lexinfo) {
   astree* tree = new astree();
   tree->symbol = symbol;
   tree->filenr = filenr;
   tree->linenr = linenr;
   tree->offset = offset;
   tree->lexinfo = intern_stringset (lexinfo);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
         tree, tree->filenr, tree->linenr, tree->offset,
         get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
         root, root->lexinfo->c_str(),
         child, child->lexinfo->c_str());
   return root;
}

astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}

astree* adopt1sym (astree* root, astree* child, int symbol) {
   root = adopt1 (root, child);
   root->symbol = symbol;
   return root;
}

/*
 * Returns true if the name of a non-terminal symbol is passed,
 * otherwise, return false.
 */
static bool is_non_term (char *tname) {
   if (strcmp (tname, "ROOT") == 0 ||
         strcmp (tname, "FUNCTION") == 0 ||
         strcmp (tname, "PROTOTYPE") == 0 ||
         strcmp (tname, "BLOCK") == 0 ||
         strcmp (tname, "CALL") == 0 ||
         strcmp (tname, "IFELSE") == 0 ||
         strcmp (tname, "RETURNVOID") == 0 ||
         strcmp (tname, "VARDECL") == 0 ||
         strcmp (tname, "PARAMLIST") == 0 ||
         strcmp (tname, "WHILE") == 0 ||
         strcmp (tname, "CONSTANT") == 0 ||
         strcmp (tname, "VARIABLE") == 0 ||
         strcmp (tname, "BINOP") == 0 ||
         strcmp (tname, "UNOP") == 0 ||
         strcmp (tname, "BASETYPE") == 0 ||
         strcmp (tname, "TYPE") == 0 ||
         strcmp (tname, "ALLOCATOR") == 0 ||
         strcmp (tname, "BLOCK") == 0 ||
         strcmp (tname, "ALLOCATOR") == 0 ||
         strcmp (tname, "IF") == 0 ||
         strcmp (tname, "IFELSE") == 0 ||
         strcmp (tname, "RETURN") == 0 ||
         strcmp (tname, "RETURNVOID") == 0 ||
         strcmp (tname, "NEWARRAY") == 0 ||
         strcmp (tname, "STRUCT") == 0) {
      return true;
   }
   return false;
}

/*
 * Converts a non-terminal symbol name to lower-case symbol name.
 */
static char* convert_non_term (char *tname) {
   if (strcmp (tname, "ROOT") == 0)
      return strdup ("program");
   if (strcmp (tname, "CALL") == 0)
      return strdup ("call");
   if (strcmp (tname, "VARDECL") == 0)
      return strdup ("vardecl");
   if (strcmp (tname, "PROTOTYPE") == 0)
      return strdup ("prototype");
   if (strcmp (tname, "STRUCT") == 0)
      return strdup ("struct");
   if (strcmp (tname, "FUNCTION") == 0)
      return strdup ("function");
   if (strcmp (tname, "PARAMLIST") == 0)
      return strdup ("parameter");
   if (strcmp (tname, "CONSTANT") == 0)
      return strdup ("constant");
   if (strcmp (tname, "VARIABLE") == 0)
      return strdup ("variable");
   if (strcmp (tname, "UNOP") == 0)
      return strdup ("unop");
   if (strcmp (tname, "BINOP") == 0)
      return strdup ("binop");
   if (strcmp (tname, "BASETYPE") == 0)
      return strdup ("basetype");
   if (strcmp (tname, "TYPE") == 0)
      return strdup ("type");
   if (strcmp (tname, "ALLOCATOR") == 0)
      return strdup ("allocator");
   if (strcmp (tname, "BLOCK") == 0)
      return strdup ("block");
   if (strcmp (tname, "ALLOCATOR") == 0)
      return strdup ("allocator");
   if (strcmp (tname, "NEWARRAY") == 0)
      return strdup ("newarray");
   if (strcmp (tname, "IF") == 0)
      return strdup ("if");
   if (strcmp (tname, "WHILE") == 0)
      return strdup ("while");
   if (strcmp (tname, "IFELSE") == 0)
      return strdup ("ifelse");
   if (strcmp (tname, "RETURN") == 0 ||
         strcmp (tname, "RETURNVOID") == 0)
      return strdup ("return");
   return tname;
}

static void dump_node (FILE* outfile, astree* node) {
   fprintf (outfile, "%p->{%s(%d) %ld:%ld.%03ld \"%s\" [",
         node, get_yytname (node->symbol), node->symbol,
         node->filenr, node->linenr, node->offset,
         node->lexinfo->c_str());
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
         ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
      fprintf (outfile, "%p", node->children.at(child));
   }
   fprintf (outfile, "]}");
}

static void dump_astree_rec (FILE* outfile, astree* root,
      int depth) {
   if (root == NULL) return;

   // Removes the TOK_ if front of symbol token name.
   char *tname = (char *) get_yytname (root->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;

   /*
    * If the debug statement was called, use Mackey's print code for
    * dumping to stederr. Else use grading scheme to dump to .ast file.
    */
   if (outfile == stderr) {
      fprintf (outfile, "%*s%s ", depth * 3, "",
            root->lexinfo->c_str());
      dump_node (outfile, root);
      fprintf (outfile, "\n");
   } else {
      /*
       * If the node is of a non-terminal just print name of
       * non-terminal. Else, print node token symbol and lexical
       * information.
       */
      if (is_non_term (tname)) {
         if (strcmp (root->lexinfo->c_str(), ";")) {
            fprintf (outfile, "%*s%s ", depth * 2, "",
                  convert_non_term (tname));
            fprintf (outfile, "\n");
         }
      } else {
         fprintf (outfile, "%*s%s (%s) ", depth * 2, "",
               get_yytname (root->symbol), root->lexinfo->c_str());
         fprintf (outfile, "\n");
      }
   }

   for (size_t child = 0; child < root->children.size();
         ++child) {
      dump_astree_rec (outfile, root->children[child],
            depth + 1);
   }
}

void dump_astree (FILE* outfile, astree* root) {
   dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}

void yyprint (FILE* outfile, unsigned short toknum,
      astree* yyvaluep) {
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      fprintf (outfile, "%s(%d)\n",
            get_yytname (toknum), toknum);
   }
   fflush (NULL);
}


void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%p]-> %d:%d.%d: %s: \"%s\")\n",
         root, root->filenr, root->linenr, root->offset,
         get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}

void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}

/*
 * Returns the parameters of the type being passed
 */
static string get_param (astree *node, size_t child) {
   string param = "";

   if (node->children.size() < child) return "";

   int param_index = 2; // Index of child[] with the parameters in it

   // Node with parameter children
   astree *parameter = node->children[param_index];
   for (size_t child = 0; child < parameter->children.size();
         ++child) {
      int check_if_array = strcmp (get_yytname
            (parameter->children[child]->children[0]->symbol),
            "TOK_ARRAY");
      if (check_if_array == 0) {
         param += parameter->children[child]->children[0]->
               children[0]->children[0]->lexinfo->c_str();
         param.append ("[]");
      } else {
         param += parameter->children[child]->children[0]->
               children[0]->lexinfo->c_str();
      }

      if (child + 1 < parameter->children.size()) param += ",";
   }

   return param;
}

/*
 * Adds parameters to the symbol table.
 */
static void add_param_sym (SymbolTable *table, astree *node,
      size_t child) {
   if (node->children.size() < child) return;

   int fn_index = 2;
   int struct_index = 1;
   int param_index = 0; // Index of child[] with the parameters in it
   astree *insert_sym = NULL;

   if (child ==  FN_WITH_PARAM || child == PROTO_WITH_PARAM)
      param_index = fn_index;
   else
      param_index = struct_index;

   // Node with parameter children
   astree *parameter = node->children[param_index];
   for (size_t child = 0; child < parameter->children.size();
         ++child) {
      int check_if_array = strcmp (get_yytname
            (parameter->children[child]->children[0]->symbol),
            "TOK_ARRAY");
      insert_sym = parameter->children[child]->children[1];
      string lexinfo = insert_sym->lexinfo->c_str();

      string type = "";
      if (check_if_array == 0) {
         type = parameter->children[child]->children[0]->
               children[0]->children[0]->lexinfo->c_str();
         type.append ("[]");
      } else {
         type = parameter->children[child]->children[0]->
               children[0]->lexinfo->c_str();
      }

      table->addSymbol (lexinfo, type, insert_sym);
   }
}

string get_type (astree *node) {
   string type = "";
   int check_if_array = strcmp (get_yytname
         (node->children[0]->children[0]->symbol), "TOK_ARRAY");

   if (check_if_array == 0) {
      type = node->children[0]->children[0]->
            children[0]->children[0]->lexinfo->c_str();
      type.append ("[]");
   } else {
      type = node->children[0]->children[0]->
            children[0]->lexinfo->c_str();
   }

   return type;
}

/*
 * Traverses through the AST to build the symbol table.
 */
static void traverse_ast_rec (SymbolTable *table, SymbolTable *types,
      astree* root, int depth) {
   if (root == NULL) return;

   astree *insert_sym = NULL;
   int cmp_func = strcmp ((char *)get_yytname (root->symbol),
         "TOK_FUNCTION") == 0;
   int cmp_proto = strcmp ((char *)get_yytname (root->symbol),
         "TOK_PROTOTYPE") == 0;
   int cmp_vardecl = strcmp ((char *)get_yytname (root->symbol),
         "TOK_VARDECL") == 0;
   int cmp_while = strcmp ((char *)get_yytname (root->symbol),
         "TOK_WHILE") == 0;
   int cmp_struct = strcmp (get_yytname (root->symbol),
         "TOK_STRUCT") == 0;
   int cmp_if = strcmp ((char *)get_yytname (root->symbol),
         "TOK_IF") == 0;
   int cmp_ifelse = strcmp ((char *)get_yytname (root->symbol),
         "TOK_IFELSE") == 0;

   // If the node in the AST is a function
   if (cmp_func || cmp_proto) {
      int name_i = 1;  // Index of child[] for the name of function

      string param = get_type (root);
      param.append ("(");
      if (cmp_func)
         param.append (get_param(root, FN_WITH_PARAM));
      else param.append (get_param(root, PROTO_WITH_PARAM));

      param.append (")");
      insert_sym = root->children[name_i];
      string lexinfo = insert_sym->lexinfo->c_str();

      table = table->enterFunction (lexinfo, param, insert_sym);
      root->blockNum = table->N - 1;
      if (cmp_func) {
         add_param_sym (table, root, FN_WITH_PARAM);
      } else {
         add_param_sym (table, root, PROTO_WITH_PARAM);
      }
   } else if (cmp_vardecl) {
      int name_i = 1;  // Index of child[] for the name of function

      insert_sym = root->children[name_i];
      string lexinfo = insert_sym->lexinfo->c_str();
      string type = get_type (root);
      table->addSymbol (lexinfo, type, insert_sym);
   } else if (cmp_while || cmp_if || cmp_ifelse) {
      table = table->enterBlock();
      root->blockNum = table->N - 1;
   } else if (cmp_struct) {
      insert_sym = root->children[0];
      string lexinfo = insert_sym->lexinfo->c_str();
      types = types->enterFunction (lexinfo, "struct", insert_sym);
      add_param_sym (types, root, STRUCT_WITH_PARAM);
   }

   for (size_t child = 0; child < root->children.size();
         ++child) {
      traverse_ast_rec (table, types, root->children[child],
            depth + 1);
   }

   if (cmp_func || cmp_proto || cmp_while || cmp_if || cmp_ifelse)
      table = table->getParent();
   else if (cmp_struct)
      types = types->getParent();
}

void traverse_ast (SymbolTable *global, SymbolTable *types,
      astree* root) {
   traverse_ast_rec (global, types, root, 0);
   fflush (NULL);
}
