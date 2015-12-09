#define USER_CONFIG_LOCATION ".config/sysgeep/"
#define ROOT_CONFIG_LOCATION "/etc/sysgeep/"
#define CONFIG_FILE_NAME "sysgeep.conf"

int sysgeep_setup(char * local_git_repo_path)
{
   int ret = 0;

   //
   //if no option -r, check whether the user is root
   //if root, use /etc/sysgeep/sysgeep.conf
   //if not root, use 

   return ret;
}

int sysgeep_save(char * file_path)
{
   int ret = 0;
   return ret;
}

int sysgeep_restore(char * file_path)
{
   int ret = 0;
   return ret;
}
