#include <stdlib.h> // getenv(), abort()
#include <stdio.h>  // fprintf()
#include <string.h> // strlen()
#include <sys/stat.h>

#define USER_CONFIG_FILE "sysgeep.conf"
#define USER_CONFIG_DIR ".config/sysgeep/"
#define USER_CONFIG_LOCATION USER_CONFIG_DIR##USER_CONFIG_FILE
#define ROOT_CONFIG_LOCATION "/etc/sysgeep/sysgeep.conf"

//if option -s, use /etc/sysgeep.conf
//else, use $HOME/.conf/sysgeep/sysgeep.conf
char * config_file(int sflag)
{
   char * config_file_path;

   if (sflag)
   {
      config_file_path = ROOT_CONFIG_LOCATION;
   }
   else
   {
      char * homedir = getenv ("HOME");
      if (homedir == NULL)
      {
         fprintf(stderr, "Error: HOME variable is not set.\n");
         fprintf(stderr, "try -s option to use system wide "ROOT_CONFIG_LOCATION"\n");
         abort();
      }
      config_file_path = malloc(sizeof(char)*(strlen(USER_CONFIG_LOCATION) + strlen(homedir) + 1));
      sprintf(config_file_path, "%s/"USER_CONFIG_LOCATION, homedir);

      // create directory if missing
      char * directory_to_check = malloc(sizeof(char)*(strlen("/.config") + strlen(homedir) + 1));
      sprintf(directory_to_check, "%s/.config", homedir);
      struct stat s;
      if (!(stat(directory_to_check, s) == 0 && S_ISDIR(s.st_mode)))
      {
        mode_t old_mask = umask(0);
        int md_ret = mkdir(USER_CONFIG_DIR, "700");
        unmask(old_mask);
      }
   }
}

void create_config_dir(int sflag)
{
   if (!sflag) // case of $HOME/.conf/sysgeep/sysgeep.conf
   {
      struct stat s;
      stat("", s)
   }
}

int sysgeep_setup(char * local_git_repo_path, int sflag)
{
   int ret = 0;
   char * config_file = config_file(sflag);
   create_config_dir(sflag);

   //

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
