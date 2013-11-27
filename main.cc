// Author: Paul Scherer, pscherer@ucsc.edu

#include <algorithm>
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

string check_binop (astree* node, SymbolTable* types,
      SymbolTable* global);
string check_newarray (astree* node, SymbolTable* types,
      SymbolTable* global);

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

string are_compatible (string name1, string name2, size_t linenr) {
   if (strcmp (name1.c_str(), "bool") == 0) {
      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name1.c_str(), name2.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to bool\n", linenr);
      } else {
         return "bool";
      }
   }

   if (strcmp (name1.c_str(), "int") == 0) {
      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name1.c_str(), name2.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to int\n", linenr);
      } else {
         return "int";
      }
   }

   if (strcmp (name1.c_str(), "string") == 0) {
      if (strcmp (name2.c_str(), "") == 0) {
         errprintf ("%zu: %s not found\n", linenr, name2.c_str());
         set_exitstatus (EXIT_FAILURE);
      }
      int comp = strcmp (name1.c_str(), name2.c_str());

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to string\n", linenr);
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

      if (comp != 0) {
         errprintf ("%zu: Invalid conversion to char\n", linenr);
      } else {
         return "char";
      }
   }

   if (strcmp (name1.c_str(), name2.c_str()) == 0) {
      return name1;
   }

   set_exitstatus (EXIT_FAILURE);
   return false;
}

string check_constant (astree* node, SymbolTable* types,
      SymbolTable* global) {
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

bool is_int (astree* node, SymbolTable* types, SymbolTable* global) {
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CONSTANT") == 0) {
      string const_type = get_yytname (node->children[0]->symbol);

      if (strcmp (const_type.c_str(), "NUMBER") == 0) {
         return true;
      }

   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARIABLE") == 0) {
      int field_index = 1;
      string vari_name = node->children[0]->lexinfo->c_str();
      string const_type = "";

      if (strcmp (vari_name.c_str(), ".") == 0) {
         astree* fn = node->children[0]->children[0]->children[0];
         string fn_name = fn->lexinfo->c_str();
         string fn_type = global->lookup(fn_name, node->linenr);
         SymbolTable* getScope = types->lookup_param(fn_type,
               node->linenr);

         if (getScope != NULL) {
            astree* binop_name = node->children[0]->
                  children[field_index];
            vari_name = binop_name->lexinfo->c_str();
            const_type = getScope->lookup(vari_name, node->linenr);
         }
      } else {
         const_type = global->lookup(vari_name, node->linenr);
      }

      if (strcmp (const_type.c_str(), "int") == 0) {
         return true;
      }
   }

   return false;
}

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

   int field_index = 1;

   astree* fn = ident_name->children[0]->children[0];
   string fn_name = fn->lexinfo->c_str();
   string fn_type = global->lookup(fn_name, node->linenr);
   if (field_cmp) {
      SymbolTable* getScope = types->lookup_param(fn_type,
            node->linenr);

      if (getScope != NULL) {
         ident_name = ident_name->children[field_index];
         name = ident_name->lexinfo->c_str();
         return getScope->lookup(name, node->linenr);
      }
   } else {
      int array_index = 1;
      astree* check_int = ident_name->children[array_index];

      if (!is_int (check_int, types, global)) {
         errprintf ("%zu: Must be [int]\n", node->linenr);
         set_exitstatus (EXIT_FAILURE);
      } else {
         return fn_type;
      }
   }

   return "";
}

string check_call (astree* node, SymbolTable* types,
      SymbolTable* global) {
   astree* ident = node->children[0];
   string ident_name = ident->lexinfo->c_str();

   vector<string> type = global->parseSignature(global->
         lookup(ident_name, node->linenr));
   return type.front();
}

string check_unop (astree* node, SymbolTable* types,
      SymbolTable* global) {
   return "";
}

string check_allocator (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string vari_name = node->children[0]->lexinfo->c_str();
   string const_type = get_yytname (node->children[0]->symbol);

   if (strcmp (const_type.c_str(), "IDENT") == 0)
      return global->lookup(vari_name, node->linenr);
   else
      return vari_name;
}

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
      return check_allocator(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CALL") == 0) {
      return check_call(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARIABLE") == 0) {
      return check_variable(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_CONSTANT") == 0) {
      return check_constant(node, types, global);
   }
   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_NEWARRAY") == 0) {
      return check_newarray(node, types, global);
   }

   return "";
}

string check_binop (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string expr1 = check_expr (node->children[0], types, global);
   string expr2 = check_expr (node->children[2], types, global);
   return are_compatible(expr1, expr2, node->linenr);
}

string check_basetype (astree* node, SymbolTable* types,
      SymbolTable* global) {
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

string check_newarray (astree* node, SymbolTable* types,
      SymbolTable* global) {
   string array_size = check_expr (node->children[1], types, global);

   if (strcmp (array_size.c_str(), "int") != 0) {
      errprintf ("%zu: Must be [int]\n", node->linenr);
      set_exitstatus (EXIT_FAILURE);
   }

   string array_type = check_basetype (node->children[0], types, global);
   array_type.append ("[]");
   return array_type;
}

void typecheck_rec (astree* node, SymbolTable* types,
      SymbolTable* global, int depth) {
   if (node == NULL) return;

   for (size_t child = 0; child < node->children.size();
         ++child) {
      typecheck_rec (node->children[child], types, global, depth + 1);
   }

   if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_VARDECL") == 0) {
      int name_index = 1;
      string name = node->children[name_index]->lexinfo->c_str();
      string type = global->lookup(name, node->linenr);

      string expr = check_expr (node->children[2], types, global);

      fprintf (stderr, "%s:%s\n", type.c_str(), expr.c_str());
      are_compatible (type, expr, node->linenr);
   } else if (strcmp ((char *)get_yytname (node->symbol),
         "TOK_BINOP") == 0/* &&
         strcmp (node->children[1]->lexinfo->c_str(), "=") == 0*/) {
      string expr1 = check_expr (node->children[0], types, global);
      string expr2 = check_expr (node->children[2], types, global);

      fprintf (stderr, "%s:%s\n", expr1.c_str(), expr2.c_str());
      are_compatible (expr1, expr2, node->linenr);
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
