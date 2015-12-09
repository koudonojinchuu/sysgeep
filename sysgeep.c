// Sysgeep, Samuel Amo 2015. GPLv3.

#include <stdio.h>
#define VERSION "20151209"

static void usage(void)                                                                               
{  
   fprintf(stderr, "sysgeep - Keep System Config in Git, version "VERSION);
   fprintf(stderr, "Usage:\n");
   fprintf(stderr, "\tsysgeep --help\n");
   fprintf(stderr, "\tsysgeep setup <path-to-local-git-repository>\n");
   fprintf(stderr, "\tsysgeep save <path-to-file>\n");
   fprintf(stderr, "\tsysgeep restore <path-to-file>\n");
}

static unsigned int args_missing(unsigned int argc_remaining, unsigned int nb_missing)
{
   if (argc_remaining < nb_missing)
   {
      fprintf(stderr, "Error, you need %d more argument(s).\n", nb_missing);
      usage();
      return 1;
   }
   return 0;
}

int main(unsigned int argc, char ** argv)
{
   unsigned int argc_remaining = argc - 1;
   if(args_missing(argc_remaining--, 1)) return -1;

   int ret = 0;

   if (strcmp("--help", argv[1]))
   {
      usage();
   }
   else if (strcmp("setup"))
   {
      if(args_missing(argc_remaining, 1)) return -1;

      ret = sysgeep_setup(argv[2]);
   }
   else if (strcmp("save"))
   {
      if (args_missing(argc_remaining, 1)) return -1;

      ret = sysgeep_save(argv[2]);
   }
   else if (strcmp("restore"))
   {
      if (args_missing(argc_remaining, 1)) return -1;

      ret = sysgeep_restore(argv[2]);
   }
   else
   {
      fprintf(stderr, "Error, unknown action.");
      usage();
      ret = -1;
   }

   return ret;
}
