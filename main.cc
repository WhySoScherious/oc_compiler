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
   size_t period_pos;
   string delim = "\\ \t\n";
   char *token;

   int pathindex = argc - 1;
   string path = argv[pathindex];

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

   FILE *oc_file;
   oc_file = fopen (path.c_str(), "r");

   if (oc_file == NULL) {
      fprintf (stderr, "Cannot access '%s': No such file or"
            "directory.\n", path.c_str());
      exit (EXIT_FAILURE);
   }

   string at_value = "";
   string dvalue = "";
   int lflag = 0;
   int yflag = 0;

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

   system (("cpp " + path + " " + cpp_filename).c_str());
   fclose (oc_file);

   FILE *cpp_file;
   cpp_file = fopen (cpp_filename.c_str(), "r");
   if (cpp_file == NULL) {
         fprintf (stderr, "Cannot access '%s': No such file or"
               "directory.\n", cpp_filename.c_str());
         exit (EXIT_FAILURE);
   }

   char buffer[LINESIZE];

   while (fgets (buffer, LINESIZE, cpp_file) != NULL) {
      token = strtok (buffer, delim.c_str());

      while (token != NULL) {
         intern_stringset (token);
         token = strtok (NULL, delim.c_str());
      }
   }

   fclose (cpp_file);
   dump_stringset (stdout);
   return EXIT_SUCCESS;
}

