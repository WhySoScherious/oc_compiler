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
         strcmp (tname, "NEW") == 0 ||
         strcmp (tname, "BLOCK") == 0 ||
         strcmp (tname, "ALLOCATOR") == 0 ||
         strcmp (tname, "IF") == 0 ||
         strcmp (tname, "IFELSE") == 0 ||
         strcmp (tname, "RETURN") == 0 ||
         strcmp (tname, "RETURNVOID") == 0){
      return true;
   }
   return false;
}

static char* convert_non_term (char *tname) {
   if (strcmp (tname, "ROOT") == 0)
      return strdup ("program");
   if (strcmp (tname, "CALL") == 0)
      return strdup ("call");
   if (strcmp (tname, "VARDECL") == 0)
      return strdup ("vardecl");
   if (strcmp (tname, "PROTOTYPE") == 0)
      return strdup ("prototype");
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
   if (strcmp (tname, "NEW") == 0)
      return strdup ("new");
   if (strcmp (tname, "BLOCK") == 0)
      return strdup ("block");
   if (strcmp (tname, "ALLOCATOR") == 0)
      return strdup ("allocator");
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

   char *tname = (char *) get_yytname (root->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;

   if (outfile == stderr) {
      fprintf (outfile, "%*s%s ", depth * 3, "",
            root->lexinfo->c_str());
      dump_node (outfile, root);
      fprintf (outfile, "\n");
   } else {
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
