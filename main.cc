// Author: Paul Scherer, pscherer@ucsc.edu

#include <string>
using namespace std;

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringset.h"

int main (int argc, char **argv) {
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
               fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else
               fprintf (stderr, "Unknown option '-%c'.\n", optopt);
            return 1;
         default:
            abort ();
      }
   }

   for (int i = 1; i < argc; ++i) {
      const string* str = intern_stringset (argv[i]);
      printf ("intern (\"%s\") returned %p->\"%s\"\n",
              argv[i], str->c_str(), str->c_str());
   }
   dump_stringset (stdout);
   return EXIT_SUCCESS;
}

