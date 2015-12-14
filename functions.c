#include <stdlib.h> // getenv(), abort(), realpath()
#include <stdio.h>  // fprintf()
#include <string.h> // strlen()
#include <sys/stat.h>// stat()
#include <error.h>  // error()
#include <unistd.h> // access()
#include <libgen.h> // dirname(), basename()
#include <git2.h>   // use git without system() calls

#define USER_CONFIG_FILE "sysgeep.conf"
#define USER_CONFIG_DIR ".config/sysgeep/"
#define USER_CONFIG_LOCATION USER_CONFIG_DIR USER_CONFIG_FILE
#define ROOT_CONFIG_LOCATION "/etc/sysgeep.conf"

//if option -s, use /etc/sysgeep.conf
//else, use $HOME/.conf/sysgeep/sysgeep.conf
FILE * config_file(int sflag, char rw)
{
   char * config_file_path;
   struct stat s;

   if (sflag)
   {
      config_file_path = ROOT_CONFIG_LOCATION;
   }
   else
   {
      char * homedir = getenv ("HOME");
      if (homedir == NULL)
         error(1, 1, "Error: HOME variable is not set.\n"
               "try -s option to use system wide "ROOT_CONFIG_LOCATION);

      config_file_path = malloc(sizeof(char)*(strlen(USER_CONFIG_LOCATION) + strlen(homedir) + 1));
      sprintf(config_file_path, "%s/"USER_CONFIG_LOCATION, homedir);

      // create directories if missing
      char * directory_to_check = malloc(sizeof(char)*(strlen("/.config/sysgeep") + strlen(homedir) + 1));
      mode_t old_mask = umask(0);
      sprintf(directory_to_check, "%s/.config", homedir);

      if (!(stat(directory_to_check, &s) == 0 && S_ISDIR(s.st_mode)))
         if (mkdir(directory_to_check, 0700))
            error(1, 1, "Error: could not create %s/.config directory", homedir);
      sprintf(directory_to_check, "%s/.config/sysgeep", homedir);

      if (!(stat(directory_to_check, &s) == 0 && S_ISDIR(s.st_mode)))
         if (mkdir(directory_to_check, 0700))
            error(1, 1, "Error: could not create %s/.config/sysgeep directory", homedir);
      umask(old_mask);

      free(directory_to_check);
   }
   // open the config file and return its file descriptor
   FILE * config_file_path_fd;
   switch (rw)
   {
      case 'r' :
         if (access(config_file_path, R_OK))
            error(1, 1, "Error: cannot read in %s", config_file_path);
         config_file_path_fd = fopen(config_file_path, "r");
         break;
      case 'w' :
         // if needed,check wether we have write permission in /etc
         if (sflag)
            if (access("/etc", W_OK))
               error(1, 1, "Error: cannot write in /etc");
         config_file_path_fd = fopen(config_file_path, "w+");
         break;
      default :
         abort();
   }
   free(config_file_path);
   return config_file_path_fd;
}

int sysgeep_setup(char * local_git_repo_path, int sflag)
{
   // check if local_git_repo_path is a git repo
   char * git_repo_check = malloc(sizeof(char)*(strlen(local_git_repo_path) + strlen("/.git")));
   sprintf(git_repo_check, "%s/.git", local_git_repo_path);
   struct stat s;
   if (!(stat(git_repo_check, &s) == 0 && S_ISDIR(s.st_mode)))
      error(1, 1, "Error: %s is not a git repository", local_git_repo_path);
   free(git_repo_check);

   // copy the git repo name into the config file
   FILE * config_file_fd = config_file(sflag, 'w');
   fprintf(config_file_fd, local_git_repo_path);
   fclose(config_file_fd);
   return 0;
}

char * get_git_repo(int sflag)
{
   char * git_repo = NULL;
   size_t n = 0;
   FILE * config_file_fd = config_file(sflag, 'r');
   getline(&git_repo, &n, config_file_fd);   
   fclose(config_file_fd);
   return git_repo;
}

// make (recursively with its parents) a directory if it does not exist yet
void recurs_make_dirs(char * dir_path)
{
   // if dir exist, exit
   struct stat s;
   if (stat(dir_path, &s) == 0 && S_ISDIR(s.st_mode)) return;
   // if dir does not exists,
   // make its parent if it (the parent) does not exist
   char * arg_dirname = strdup(dir_path);
   recurs_make_dirs(dirname(arg_dirname));
   free(arg_dirname);
   // make the (child) directory
   mode_t old_mask = umask(0);
   if (mkdir(dir_path, 0700))
      error(1, 1, "Error: could not create directory %s", dir_path);
   umask(old_mask);
}

// XXX
int sysgeep_save(char * file_path, int sflag)
{
   char * git_repo_path = get_git_repo(sflag);

   // get absolute path of "file_path"
   char * abs_path = realpath(file_path, NULL);

   // make it a path inside the git repo
   char * in_git_path = malloc(sizeof(char)*(strlen(abs_path) + strlen(git_repo_path)));
   sprintf(in_git_path, "%s%s", git_repo_path, abs_path);
   free(abs_path);

   // check and create all the parent directories of "file_path" inside git repo
   char * arg_dirname = strdup(in_git_path);
   char * parent_dir = dirname(arg_dirname);
   recurs_make_dirs(parent_dir);
   free(arg_dirname);

   // create or update the file copied from "file_path" in the git repo
   FILE * file_to_write = fopen(in_git_path, "w+");
   FILE * src = fopen(file_path, "r");
   char c;
   while( (c = fgetc(src)) != EOF ) fputc(c, target); 
   fclose(src);
   fclose(file_to_write);

   git_threads_init();

   // be sure the given path will be relative: no '/' at the begining
   int i = 0;
   while ( (i < pathlen) && (file_path[i] == '/') ) ++i;
   char * file_path_rel = file_path + i;

   // add the file to the index
   git_repository * repo = NULL;
   if (git_repository_open_ext(&repo, git_repo_path))
      error(1, 1, "Error: could not open git repository: %s", git_repo_path);
   git_index * idx = NULL;
   if (git_repository_index(&idx, repo))
      error(1, 1, "Error: could not open index of repository: %s", git_repo_path);
   if (git_index_add_bypath(idx, file_path))

   // take note of the permissions and owner:group
   // keep them on a line in a lexicographic ordered file at root of the git repo
   // TODO
   
   // commit with the file name as message
   // TODO

   printf("%s\n", git_repo_path);

   free(in_git_path);
   git_index_free(idx);
   git_repository_free(repo);
   git_threads_shutdown();
   free(git_repo_path);
   return 0;
}

// XXX
int sysgeep_restore(char * file_path, int sflag)
{
   int ret = 0;
   return ret;
}
