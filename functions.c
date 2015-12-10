#include <stdlib.h> // getenv(), abort()
#include <stdio.h>  // fprintf()
#include <string.h> // strlen()

#define USER_CONFIG_LOCATION ".config/sysgeep/sysgeep.conf"
#define ROOT_CONFIG_LOCATION "/etc/sysgeep/sysgeep.conf"

//if option -s, use /etc/sysgeep.conf
//else, use $HOME/.conf/sysgeep/sysgeep.conf
char * config_file(int sflag)
{
   if (sflag)
   {
      return ROOT_CONFIG_LOCATION;
   }
   else
   {
      char * homedir = getenv ("HOME");
      if (homedir == NULL)
      {
         fprintf(stderr, "Error: HOME variable is not set.\n");
         fprintf(stderr, "try -s option to use system wide /etc/sysgeep/sysgeep.conf\n");
         abort();
      }
      char * config_file_path = malloc(sizeof(char)*(strlen(USER_CONFIG_LOCATION) + strlen(homedir) + 1));
      sprintf(config_file_path, "%s/"USER_CONFIG_LOCATION, homedir);
      return config_file_path;
   }
}

int sysgeep_setup(char * local_git_repo_path, int sflag)
{
   int ret = 0;
   char * config_file = config_file(sflag);

   return ret;
}

int sysgeep_save(char * file_path, int sflag)
{
   int ret = 0;
   return ret;
}

int sysgeep_restore(char * file_path, int sflag)
{
   int ret = 0;
   return ret;
}
