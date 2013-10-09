// Author: Paul Scherer, pscherer@ucsc.edu

#include <string>
using namespace std;

#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringset.h"

const size_t LINESIZE = 1024;

int main (int argc, char **argv) {
   string cpp_filename = "cpp_output";
   string prog_name;          // Name of program passed
   size_t period_pos;         // Index of period for file extension
   string delim = "\\ \t\n";
   char *token;               // Tokenized string

   int pathindex = argc - 1;        // Index of path from arguments
   string path = argv[pathindex];   // Path name of .oc file

   // Check if a valid .oc file was passed.
   period_pos = path.find_last_of (".");
   if (period_pos == string::npos) {
      fprintf (stderr, "Must pass .oc file.\n");
      exit (EXIT_FAILURE);
   }

   string file_type = path.substr(period_pos + 1);
   if (file_type.compare ("oc") != 0) {
      fprintf (stderr, "Invalid file type '.%s'.\n", file_type.c_str());
      exit (EXIT_FAILURE);
   }

   // Get program name.
   char *cp_path = strdup (path.c_str());
   prog_name = basename (cp_path);
   prog_name = prog_name.substr (0, period_pos);
   free (cp_path);

   // Check if file specified exists, and open it.
   FILE *oc_file;
   oc_file = fopen (path.c_str(), "r");
   if (oc_file == NULL) {
      fprintf (stderr, "Cannot access '%s': No such file or"
            "directory.\n", path.c_str());
      exit (EXIT_FAILURE);
   }

   // Flags for option parameters passed.
   string at_value = "";
   string dvalue = "";
   int lflag = 0;
   int yflag = 0;

   // Activate flags if passed in command arguments.
   int c;
   while ((c = getopt (argc, argv, "@:D:ly")) != -1) {
      switch (c) {
         case '@':
            at_value = optarg;
            break;
         case 'D':
            dvalue = optarg;
            break;
         case 'l':
            lflag = 1;
            break;
         case 'y':
            yflag = 1;
            break;
         case '?':
            if (optopt == '@' || optopt == 'D')
               fprintf (stderr, "Option -%c requires an argument.\n",
                     optopt);
            else
               fprintf (stderr, "Unknown option '-%c'.\n", optopt);
            return 1;
         default:
            exit (EXIT_FAILURE);
      }
   }

   // If the -D option was passed, pass option for cpp command, else
   // call "cpp infile outfile".
   if (dvalue.compare("") != 0)
      system (("cpp -D " + dvalue + " " + path + " " +
            cpp_filename).c_str());
   else
      system (("cpp " + path + " " + cpp_filename).c_str());
   fclose (oc_file);

   // Check for a valid cpp output file and open it.
   FILE *cpp_file;
   cpp_file = fopen (cpp_filename.c_str(), "r");
   if (cpp_file == NULL) {
         fprintf (stderr, "Cannot access '%s': No such file or"
               "directory.\n", cpp_filename.c_str());
         exit (EXIT_FAILURE);
   }

   // Create a program.str file
   FILE *str_file = fopen ((prog_name + ".str").c_str(), "w");

   // Read line of cpp output file and tokenize it, and insert it into
   // the string set.
   char buffer[LINESIZE];
   while (fgets (buffer, LINESIZE, cpp_file) != NULL) {
      token = strtok (buffer, delim.c_str());

      while (token != NULL) {
         const string* str = intern_stringset (token);
         fprintf (str_file, "intern (\"%s\") returned %p->\"%s\"\n",
                       token, str->c_str(), str->c_str());
         token = strtok (NULL, delim.c_str());
      }
   }

   fclose (cpp_file);

   // Dump the string set into the file.
   dump_stringset (str_file);
   fclose (str_file);

   return EXIT_SUCCESS;
}

