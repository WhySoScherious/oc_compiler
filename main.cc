// Author: Paul Scherer, pscherer@ucsc.edu

#include <string>
#include <vector>
using namespace std;

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "astree.h"
#include "auxlib.h"
#include "lyutils.h"
#include "stringset.h"
#include "symtable.h"

const size_t LINESIZE = 1024;
string dvalue = "";        // Flag for option parameter passed.
string prog_name;          // Name of program passed

const string CPP = "/usr/bin/cpp";

void yyin_cpp_popen (const char* filename) {
   string command = "";
   // If the -D option was passed, pass option for cpp command, else
   // call "cpp infile outfile".
   string cpp_d_command = CPP + " -D " + dvalue + " " + filename;
   string cpp_command = CPP + " " + filename;
   if (dvalue.compare("") != 0)
      command = cpp_d_command;
   else
      command = cpp_command;

   yyin = popen (command.c_str(), "r");

   // Exit failure if the pipe doesn't open
   if (yyin == NULL) {
      syserrprintf (command.c_str());
      exit (get_exitstatus());
   }
}

void scan_opts (int argc, char** argv) {
   // Activate flags if passed in command arguments.
   opterr = 0;
   yy_flex_debug = 0;
   yydebug = 0;
   int c;
   while ((c = getopt (argc, argv, "@:D:ly")) != -1) {
      switch (c) {
         case '@': set_debugflags (optarg);  break;
         case 'D': dvalue = optarg;          break;
         case 'l': yy_flex_debug = 1;        break;
         case 'y': yydebug = 1;              break;
         default:  errprintf ("%:bad option (%c)\n", optopt); break;
      }
   }

   if (optind > argc) {
      errprintf ("Usage: %s [-ly] [filename]\n", get_execname());
      exit (get_exitstatus());
   }

   const char* filename = optind == argc ? "-" : argv[optind];
   open_tok_file (prog_name);

   yyin_cpp_popen (filename);
   DEBUGF ('m', "filename = %s, yyin = %p, fileno (yyin) = %d\n",
         filename, yyin, fileno (yyin));
   scanner_newfilename (filename);
}

void insert_stringset() {
   char *token;               // Tokenized string
   string delim = "\\ \t\n";

   // Create a program.str file
   FILE *str_file = fopen ((prog_name + ".str").c_str(), "w");

   // Read line of cpp output file and tokenize it, and insert it into
   // the string set.
   char buffer[LINESIZE];
   while (fgets (buffer, LINESIZE, yyin) != NULL) {
      token = strtok (buffer, delim.c_str());

      while (token != NULL) {
         intern_stringset (token);
         token = strtok (NULL, delim.c_str());
      }
   }

   // Dump the string set into the file.
   dump_stringset (str_file);
   fclose (str_file);
}

string get_basetype (astree* node) {
   if (strcmp ((char *)get_yytname (node->symbol),
            "TOK_VOID") == 0) {
      return "void";
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BOOL") == 0) {
      return "bool";
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CHAR") == 0) {
      return "char";
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_INT") == 0) {
      return "int";
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_STRING") == 0) {
      return "string";
   }

   return NULL;
}

string get_child_type (astree *node) {
   string type = "";
   int check_if_array = strcmp (get_yytname
         (node->children[0]->symbol), "TOK_ARRAY");

   if (check_if_array == 0) {
      type = node->children[0]->children[0]->children[0]->
            lexinfo->c_str();
      type.append ("[]");
   } else {
      type = node->children[0]->children[0]->lexinfo->c_str();
   }

   return type;
}

void check_compatible (string name, string expr_name, string expr_type,
      SymbolTable* global, size_t linenr) {
   if (strcmp (name.c_str(), "bool") == 0) {
      expr_type = global->lookup(expr_name, linenr);
      if (strcmp (expr_name.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, expr_name.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name.c_str(), expr_type.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to bool\n", linenr);
         set_exitstatus (EXIT_FAILURE);
      }
   } else if (strcmp (name.c_str(), "char") == 0) {
      expr_type = global->lookup(expr_name, linenr);
      if (strcmp (expr_name.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, expr_name.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name.c_str(), expr_type.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to char\n", linenr);
         set_exitstatus (EXIT_FAILURE);
      }
   } else if (strcmp (name.c_str(), "int") == 0) {
      if (strcmp (expr_type.c_str(), "NUMBER") == 0) return;

      expr_type = global->lookup(expr_name, linenr);
      if (strcmp (expr_name.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, expr_name.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name.c_str(), expr_type.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to int\n", linenr);
         set_exitstatus (EXIT_FAILURE);
      }
   } else if (strcmp (name.c_str(), "string") == 0) {
      expr_type = global->lookup(expr_name, linenr);
      if (strcmp (expr_name.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, expr_name.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name.c_str(), expr_type.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to string\n", linenr);
         set_exitstatus (EXIT_FAILURE);
      }
   }
}

void typecheck_node (string name, astree* node, SymbolTable* types,
      SymbolTable* global) {
   string type = global->lookup(name, node->linenr);

   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CONSTANT") == 0) {
      string const_name = node->children[0]->lexinfo->c_str();
      string const_type = get_yytname (node->children[0]->symbol);

      check_compatible(type, const_name, const_type, global,
            node->linenr);
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARIABLE") == 0) {
      int field_index = 1;
      string vari_name = node->children[0]->lexinfo->c_str();
      string const_type = get_yytname (node->children[0]->symbol);

      if (strcmp (vari_name.c_str(), ".") == 0) {
         astree* fn = node->children[0]->children[0]->children[0];
         string fn_name = fn->lexinfo->c_str();
         string fn_type = global->lookup(fn_name, node->linenr);
         SymbolTable* getScope = types->lookup_param(fn_type,
               node->linenr);

         if (getScope != NULL) {
            astree* binop_name = node->children[0]->children[field_index];
            vari_name = binop_name->lexinfo->c_str();
            const_type = getScope->lookup(vari_name, node->linenr);
         }
      }

      check_compatible(type, vari_name, const_type, global,
            node->linenr);
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BINOP") == 0) {
      int var2_index = 2;
      astree* variable1 = node->children[0]->children[0];
      astree* variable2 = node->children[var2_index]->children[0];
      string vari1_name = variable1->lexinfo->c_str();
      string const1_type = get_yytname (variable1->symbol);
      string vari2_name = variable2->lexinfo->c_str();
      string const2_type = get_yytname (variable2->symbol);

      check_compatible(type, vari1_name, const1_type, global,
            node->linenr);
      check_compatible(type, vari2_name, const2_type, global,
            node->linenr);
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_ALLOCATOR") == 0) {
      string vari_name = node->children[0]->lexinfo->c_str();
      string const_type = get_yytname (node->children[0]->symbol);

      check_compatible(type, vari_name, const_type, global,
            node->linenr);
   }
}

void typecheck_second_binop (string name, astree* node, SymbolTable* types,
      SymbolTable* global) {
   int var2_index = 2;
   string type = global->lookup(name, node->linenr);
   astree* variable2 = node->children[var2_index]->children[0];
   string vari2_name = variable2->lexinfo->c_str();
   string const2_type = get_yytname (variable2->symbol);

   check_compatible(type, vari2_name, const2_type, global,
         node->linenr);
}

void binop_rec (string name, astree* expr, SymbolTable* types,
      SymbolTable* global, int depth, int numBinops) {
   astree* binop_child = expr->children[0];

   if (strcmp ((char *)get_yytname (binop_child->symbol), "TOK_BINOP") == 0)
      binop_rec (name, binop_child, types, global, ++depth, numBinops);

   if (depth == numBinops - 1)
      typecheck_node(name, expr, types, global);
   else
      typecheck_second_binop (name, expr, types, global);
}

void typecheck_rec (astree* node, SymbolTable* types,
      SymbolTable* global, int depth) {
   if (node == NULL) return;

   for (size_t child = 0; child < node->children.size();
         ++child) {
      typecheck_rec (node->children[child], types, global, depth + 1);
   }

   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CALL") == 0) {
      if (node->children.size() == 1) return;

      int name_index = 0;
      int expr_index = 1;
      string name = node->children[name_index]->lexinfo->c_str();
      string type = global->lookup(name, node->linenr);

      typecheck_node(name, node->children[expr_index], types, global);
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARDECL") == 0) {
      string type = "";
      int name_index = 1;
      int expr_name_index = 2;
      string name = node->children[name_index]->lexinfo->c_str();
      type = global->lookup(name, node->linenr);

      int numBinops = 0;
      for (astree* index = node->children[expr_name_index];
            strcmp ((char *)get_yytname (index->symbol), "TOK_BINOP") == 0; ) {
         numBinops++;
         index = index->children[0];
      }

      astree* expr = node->children[expr_name_index];

      if (numBinops > 0) {
         binop_rec (name, expr, types, global, 0, numBinops);
      } else {
         typecheck_node(name, expr, types, global);
      }
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BINOP") == 0) {
      int field_index = 1;
      int expr_name_index = 2;
      string name = "";
      string type = "";
      astree* binop_name = node->children[0]->children[0];

      if (strcmp (binop_name->lexinfo->c_str(), ".") == 0) {
         astree* fn = binop_name->children[0]->children[0];
         string fn_name = fn->lexinfo->c_str();
         string fn_type = global->lookup(fn_name, node->linenr);
         SymbolTable* getScope = types->lookup_param(fn_type,
               node->linenr);

         if (getScope != NULL) {
            binop_name = binop_name->children[field_index];
            name = binop_name->lexinfo->c_str();
            type = getScope->lookup(name, node->linenr);
         }
      } else {
         name = binop_name->lexinfo->c_str();
         type = global->lookup(name, node->linenr);
      }

      int numBinops = 0;
      for (astree* index = node->children[expr_name_index];
            strcmp ((char *)get_yytname (index->symbol), "TOK_BINOP") == 0;) {
         numBinops++;
         index = index->children[0];
      }

      astree* expr = node->children[expr_name_index];

      if (numBinops > 0) {
         binop_rec (name, expr, types, global, 0, numBinops);
      } else {
         typecheck_node(name, expr, types, global);
      }
   }
}

int main (int argc, char **argv) {
   int parsecode = 0;
   set_execname (argv[0]);

   size_t period_pos;         // Index of period for file extension

   int pathindex = argc - 1;        // Index of path from arguments
   string path = argv[pathindex];   // Path name of .oc file

   DEBUGSTMT ('m',
      for (int argi = 0; argi < argc; ++argi) {
         eprintf ("%s%c", argv[argi], argi < argc - 1 ? ' ' : '\n');
      }
   );

   // Check if a valid .oc file was passed.
   period_pos = path.find_last_of (".");
   if (period_pos == string::npos) {
      fprintf (stderr, "Must pass .oc file.\n");
      set_exitstatus (EXIT_FAILURE);
      exit (get_exitstatus());
   }

   string file_type = path.substr(period_pos + 1);
   if (file_type.compare ("oc") != 0) {
      fprintf (stderr, "Invalid file type '.%s'.\n", file_type.c_str());
      set_exitstatus (EXIT_FAILURE);
      exit (get_exitstatus());
   }

   // Get program name.
   char *cp_path = strdup (path.c_str());
   prog_name = basename (cp_path);
   prog_name = prog_name.substr (0, period_pos);
   free (cp_path);

   // Check if file specified exists
   if (fopen (path.c_str(), "r") == NULL) {
      errprintf ("Cannot access '%s': No such file or"
            " directory.\n", path.c_str());
      exit (get_exitstatus());
   }

   scan_opts (argc, argv);

   parsecode = yyparse();

   SymbolTable *types = new SymbolTable(NULL);
   SymbolTable *global = new SymbolTable(NULL);
   FILE *ast_file = fopen ((prog_name + ".ast").c_str(), "w");
   FILE *sym_file = fopen ((prog_name + ".sym").c_str(), "w");

   if (parsecode) {
      errprintf ("%:parse failed (%d)\n", parsecode);
   } else {
      dump_astree (ast_file, yyparse_astree);
      DEBUGSTMT ('a', dump_astree (stderr, yyparse_astree); );
      traverse_ast (global, types, yyparse_astree);
      global->dump (sym_file, 0);
      types->dump (sym_file, 0);
   }

   insert_stringset();
   typecheck_rec (yyparse_astree, types, global, 0);
   fflush (NULL);

   close_tok_file ();
   fclose (ast_file);
   fclose (sym_file);

   if (pclose (yyin)) {
      set_exitstatus (EXIT_FAILURE);
      exit (get_exitstatus());
   }

   yylex_destroy();

   exit (get_exitstatus());
}
