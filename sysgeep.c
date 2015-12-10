// Sysgeep, Samuel Amo 2015. GPLv3.

#include <stdio.h>  // printf, fprintf
#include <unistd.h> // getopts
#include <stdlib.h> // abort
#define VERSION "20151209"

static void usage(void)                                                                               
{  
   fprintf(stderr, "sysgeep - Keep System Config in Git, version "VERSION"\n");
   fprintf(stderr, "Usage:\n");
   fprintf(stderr, "\tsysgeep help\n");
   fprintf(stderr, "\tsysgeep -h\n");
   fprintf(stderr, "\tsysgeep [-s] setup <path-to-local-git-repository>\n");
   fprintf(stderr, "\tsysgeep [-s] save <path-to-file>\n");
   fprintf(stderr, "\tsysgeep [-s] restore <path-to-file>\n");
}

static unsigned int args_missing(unsigned int argc_remaining, unsigned int nb_missing)
{
   if (argc_remaining < nb_missing)
   {
      fprintf(stderr, "Error: you need %d more argument(s).\n", nb_missing);
      usage();
      return 1;
   }
   return 0;
}

int main(unsigned int argc, char ** argv)
{
   int c;
   int hflag = 0;
   int sflag = 0;

   opterr = 0;
   while ((c = getopt (argc, argv, "hs")) != -1)
      switch (c)
      {
         case 'h':
            hflag = 1;
            break;
         case 's':
            sflag = 1;
            break;
         case '?':
            fprintf (stderr, "Error: unknown option `-%c'.\n", optopt);
            return 1;
         default:
            abort ();
      }

   int remaining_args = argc - optind;

   if (remaining_args == 0)
   {
      fprintf(stderr, "Error: at least one more argument is required.\n");
      usage();
      return 1;
   }

   if (hflag || !strcmp("help", argv[optind]))
   {
      usage();
      return 0;
   }

   if (!strcmp("setup", argv[optind]))
   {
      if (remaining_args < 2)
      {
         fprintf(stderr, "Error: one more argument is required.\n");
         usage();
         return 1;
      }
      return sysgeep_setup(argv[optind + 1]);
   }
   else if (!strcmp("save", argv[optind]))
   {
      if (remaining_args < 2)
      {
         fprintf(stderr, "Error: one more argument is required.\n");
         usage();
         return 1;
      }
      return sysgeep_save(argv[optind + 1]);
   }
   else if (!strcmp("restore", argv[optind]))
   {
      if (remaining_args < 2)
      {
         fprintf(stderr, "Error: one more argument is required.\n");
         usage();
         return 1;
      }
      return sysgeep_restore(argv[optind + 1]);
   }
   else
   {
      fprintf(stderr, "Error: unknown action.\n");
      usage();
      return 1;
   }

   return 0;
}
