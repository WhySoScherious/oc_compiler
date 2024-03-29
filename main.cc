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
#include "oilprint.h"
#include "stringset.h"
#include "symtable.h"
#include "typecheck.h"

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

   // Symbol table with struct types
   SymbolTable *types = new SymbolTable(NULL);

   // Global symbol table
   SymbolTable *global = new SymbolTable(NULL);
   FILE *ast_file = fopen ((prog_name + ".ast").c_str(), "w");
   FILE *sym_file = fopen ((prog_name + ".sym").c_str(), "w");
   FILE *oil_file = fopen ((prog_name + ".oil").c_str(), "w");

   if (parsecode) {
      errprintf ("%:parse failed (%d)\n", parsecode);
   } else {
      dump_astree (ast_file, yyparse_astree);
      DEBUGSTMT ('a', dump_astree (stderr, yyparse_astree); );

      // Generate the symbol table and dump to program.sym file
      traverse_ast (global, types, yyparse_astree);
      global->dump (sym_file, 0);
      types->dump (sym_file, 0);

      // Typecheck program
      typecheck_rec (yyparse_astree, types, global, 0);

      // If typecheck passed, generate the intermediate oil code
      if (get_exitstatus() == 0) {
         generate_oil (oil_file, yyparse_astree, types, global);
      }
   }

   insert_stringset();
   fflush (NULL);

   close_tok_file ();
   fclose (ast_file);
   fclose (sym_file);
   fclose (oil_file);

   // Compile the intermediate code
   string command = "gcc -g -o ";
   command.append (prog_name);
   command.append (" -x c ");
   command.append (prog_name);
   command.append (".oil oclib.c");
   system(command.c_str());

   if (pclose (yyin)) {
      set_exitstatus (EXIT_FAILURE);
      exit (get_exitstatus());
   }

   yylex_destroy();

   exit (get_exitstatus());
}
