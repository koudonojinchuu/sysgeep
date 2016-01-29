// Sysgeep, Samuel Amo 2015. GPLv3.

#include <stdio.h>  // printf(), fprintf()
#include <unistd.h> // getopts()
#include <stdlib.h> // abort()
#include <error.h>  // error()
#include <string.h> // strcmp()

#include "actions.h"

#define VERSION "20151209"

static void usage(void)                                                                               
{  
  fprintf(stderr, "sysgeep - Keep System Config in Git, version "VERSION"\n"
      "Usage:\n"
      "\tsysgeep help\n"
      "\tsysgeep -h\n"
      "\tsysgeep [-s] setup    # <- this will print your current setup\n"
      "\tsysgeep [-s] setup <path-to-local-git-repository>\n"
      "\tsysgeep [-s] setup-remote <remote-git-repo>\n"
      "\tsysgeep [-s] setup-remote <remote-git-repo> <path-to-local-git-repository>\n"
      "\tsysgeep [-s] save <path-to-file>\n"
      "\tsysgeep [-s] restore <path-to-file>\n");
}

int main(int argc, char ** argv)
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
        fprintf(stderr, "Error: unknown option `-%c'.\n", optopt);
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
      return sysgeep_print_setup(sflag);
    else
      return sysgeep_setup(argv[optind + 1], sflag);
  }
  else if (!strcmp("setup-remote", argv[optind]))
  {
    if (remaining_args == 2)
      return sysgeep_setup_remote(argv[optind + 1], sflag);
    else if (remaining_args == 3)
      return sysgeep_setup_remote_for_repo(argv[optind + 1], argv[optind + 2], sflag);
    else
    {
      fprintf(stderr, "Error: wrong number of arguments.\n");
      usage();
      return 1;
    }
  }
  else if (!strcmp("save", argv[optind]))
  {
    if (remaining_args < 2)
    {
      fprintf(stderr, "Error: one more argument is required.\n");
      usage();
      return 1;
    }
    return sysgeep_save(argv[optind + 1], sflag);
  }
  else if (!strcmp("restore", argv[optind]))
  {
    if (remaining_args < 2)
    {
      fprintf(stderr, "Error: one more argument is required.\n");
      usage();
      return 1;
    }
    return sysgeep_restore(argv[optind + 1], sflag);
  }
  else
  {
    fprintf(stderr, "Error: unknown action.\n");
    usage();
    return 1;
  }

  return 0;
}
