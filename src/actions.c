#include <stdlib.h> // getenv(), abort(), realpath()
#include <stdio.h>  // fprintf()
#include <string.h> // strlen()
#include <sys/stat.h>// stat()
#include <error.h>  // error()
#include <unistd.h> // access()
#include <libgen.h> // dirname(), basename()
#include <git2.h>   // use git without system() calls

#include "utils.h"
#include "sorted_lines.h"

#define USER_CONFIG_FILE "sysgeep.conf"
#define USER_CONFIG_DIR ".config/sysgeep/"
#define USER_CONFIG_LOCATION USER_CONFIG_DIR USER_CONFIG_FILE
#define ROOT_CONFIG_LOCATION "/etc/sysgeep.conf"

// uid_t and gid_t are 32bits uint, so their max is 10 digits
#define UIDT_MAXLEN 10
#define GIDT_MAXLEN 10
#define PERM_LEN 6

//if option -s, use /etc/sysgeep.conf
//else, use $HOME/.conf/sysgeep/sysgeep.conf
static FILE * config_file(int sflag, char rw)
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
    chk( (homedir == NULL),
        "Error: HOME variable is not set.\n"
        "try -s option to use system wide "ROOT_CONFIG_LOCATION"\n" );

    config_file_path = malloc(sizeof(char)*(strlen(USER_CONFIG_LOCATION) + strlen(homedir) + 2));
    sprintf(config_file_path, "%s/"USER_CONFIG_LOCATION, homedir);

    // create directories if missing
    char * directory_to_check = malloc(sizeof(char)*(strlen("/.config/sysgeep") + strlen(homedir) + 1));
    mode_t old_mask = umask(0);
    sprintf(directory_to_check, "%s/.config", homedir);

    if (!(stat(directory_to_check, &s) == 0 && S_ISDIR(s.st_mode)))
      chk( mkdir(directory_to_check, 0700), "Error: could not create %s/.config directory\n", homedir );
    sprintf(directory_to_check, "%s/.config/sysgeep", homedir);

    if (!(stat(directory_to_check, &s) == 0 && S_ISDIR(s.st_mode)))
      chk( mkdir(directory_to_check, 0700), "Error: could not create %s/.config/sysgeep directory\n", homedir );
    umask(old_mask);

    free(directory_to_check);
  }
  // open the config file and return its file descriptor
  FILE * config_file_path_fd;
  switch (rw)
  {
    case 'r' :
      chk( access(config_file_path, R_OK), "Error: cannot read in %s\n", config_file_path );
      config_file_path_fd = fopen(config_file_path, "r");
      break;
    case 'w' :
      // if needed,check wether we have write permission in /etc
      if (sflag)
        chk( access("/etc", W_OK), "Error: cannot write in /etc\n" );
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
  char * git_repo_check = malloc(sizeof(char)*(strlen(local_git_repo_path) + strlen("/.git") + 1));
  sprintf(git_repo_check, "%s/.git", local_git_repo_path);
  struct stat s;
  chk( !(stat(git_repo_check, &s) == 0 && S_ISDIR(s.st_mode)),
      "Error: %s is not a git repository\n", local_git_repo_path );
  free(git_repo_check);

  // if repo/.sysgeep_index not already exists, init a sysgeep_index file in the repo
  char * sysgeep_index_path = malloc(sizeof(char)*(strlen(local_git_repo_path)+strlen("/.sysgeep_index")+1));
  sprintf(sysgeep_index_path, "%s/.sysgeep_index", local_git_repo_path);
  init_counted_file(sysgeep_index_path);
  free(sysgeep_index_path);

  // copy the git repo name into the config file
  FILE * config_file_fd = config_file(sflag, 'w');
  fwrite(local_git_repo_path, 1, sizeof(char)*strlen(local_git_repo_path), config_file_fd);

  fclose(config_file_fd);
  return 0;
}

static char * get_git_repo(int sflag)
{
  char * git_repo = NULL;
  size_t n = 0;
  FILE * config_file_fd = config_file(sflag, 'r');
  getline(&git_repo, &n, config_file_fd);   
  fclose(config_file_fd);
  return git_repo;
}

// add file or directory to the sysgeep index
// the argument must be the absolute path
static void add_to_index(char * git_repo_path, char * abs_path)
{
  // if the path does not start by '/', just add it (assume it is an absolute path anyway)
  char * abs_path_dup = strdup(abs_path);
  int path_len = strlen(abs_path_dup);
  int i;
  if ((!path_len) && (abs_path_dup[0] != '/'))
  {
    abs_path_dup = realloc(abs_path_dup, path_len + 1);
    for (i=path_len;i>0;i--) abs_path_dup[i]=abs_path_dup[i-1];
    abs_path_dup[0]='/';
  }

  // path of the sysgeep index
  char * sysgeep_index_path = malloc(sizeof(char)*(strlen(git_repo_path) + strlen("/.sysgeep_index") + 1));
  sprintf(sysgeep_index_path, "%s/.sysgeep_index", git_repo_path);

  // get file (or dir) attributes and build line to put into the sysgeep index
  struct stat s;
  chk( stat(abs_path_dup, &s), "Error: could not stat() the file or directory to be saved\n" );
  char * attributes_buffer = malloc(sizeof(char)*(strlen(abs_path_dup) + UIDT_MAXLEN + GIDT_MAXLEN + PERM_LEN + 4));
  sprintf(attributes_buffer, "%s %d:%d %o", abs_path_dup, s.st_uid, s.st_gid, s.st_mode);

  // add the line to the sysgeep index
  add_sorted_line(sysgeep_index_path, attributes_buffer);

  free(attributes_buffer);
  free(sysgeep_index_path);
}

// make (recursively with its parents) a directory if it does not exist yet
// for each created directory, add it to the sysgeep index
static void recurs_make_dirs_and_add_to_index(char * dir_path, char * git_repo_path)
{
  // if dir exist, exit
  struct stat s;
  if (stat(dir_path, &s) == 0 && S_ISDIR(s.st_mode)) return;
  // if dir does not exists,
  // make its parent if it (the parent) does not exist
  char * arg_dirname = strdup(dir_path);
  recurs_make_dirs_and_add_to_index(dirname(arg_dirname), git_repo_path);
  free(arg_dirname);
  // make the (child) directory
  mode_t old_mask = umask(0);
  chk( mkdir(dir_path, 0700),
      "Error: could not create directory %s\n", dir_path );
  umask(old_mask);
  // add it to the index
  add_to_index(git_repo_path, dir_path + strlen(git_repo_path));
}

// save a given system file or dir into the sysgeep repo
int sysgeep_save(char * file_path, int sflag)
{
  char * git_repo_path = get_git_repo(sflag);

  // get absolute path of "file_path"
  char * abs_path = realpath(file_path, NULL);
  pchk_t( abs_path, "Error: could not find the real path of: %s\n", file_path );

  // make it a path inside the git repo
  char * in_git_path = malloc(sizeof(char)*(strlen(abs_path) + strlen(git_repo_path) + 1));
  sprintf(in_git_path, "%s%s", git_repo_path, abs_path);

  // remove any '/' at the begining to pass it as a relative path later
  int i = 0;
  unsigned int pathlen = strlen(abs_path);
  while ( (i < pathlen) && (abs_path[i] == '/') ) ++i;
  char * path_rel = abs_path + i;

  // check and create all the parent directories of "file_path" inside git repo
  // and for, each create directory add, it to sysgeep_index
  char * arg_dirname = strdup(in_git_path);
  char * parent_dir = dirname(arg_dirname);
  recurs_make_dirs_and_add_to_index(parent_dir, git_repo_path);
  free(arg_dirname);

  // create or update the file copied from "file_path" in the git repo
  FILE * file_to_write = fopen(in_git_path, "w+");
  FILE * src = fopen(file_path, "r");
  char c;
  while( (c = fgetc(src)) != EOF ) fputc(c, file_to_write); 
  fclose(src);
  fclose(file_to_write);

#if LIBGIT2_VER_MAJOR <= 0 && LIBGIT2_VER_MINOR < 22
  git_threads_init();
#else
  git_libgit2_init();
#endif

  // add the file to the index
  git_repository * repo = NULL;
  chk( git_repository_open(&repo, git_repo_path), "Error: could not open git repository: %s\n", git_repo_path );
  git_index * idx = NULL;
  chk( git_repository_index(&idx, repo), "Error: could not open index of repository: %s\n", git_repo_path );
  chk( git_index_add_bypath(idx, path_rel), "Error: could not add object to the index\n" );
  chk( git_index_write(idx), "Error: could not write index to disk\n" );

  // take note of the permissions and owner:group
  // keep them on a line in a lexicographic ordered file at root of the git repo
  add_to_index(git_repo_path, abs_path);

  // commit with the file name as message
  git_config * gitconfig = NULL;
  chk( git_config_open_default(&gitconfig), "Error: could not open git configuration\n" );
  git_config_entry * entry_name;
  chk( git_config_get_entry((const git_config_entry **) &entry_name, gitconfig, "user.name"), "Error: could not get user.name entry\n" );
  git_config_entry * entry_email;
  chk( git_config_get_entry((const git_config_entry **) &entry_email, gitconfig, "user.email"), "Error: could not get user.email entry\n" );
  git_signature * me = NULL;
  chk( git_signature_now(&me, entry_name->value, entry_email->value), "Error: could not create commit signature\n" );
#if LIBGIT2_VER_MAJOR >= 0 && LIBGIT2_VER_MINOR >= 23
  git_config_entry_free(entry_name); //API available from libgit2 v0.23.0
  git_config_entry_free(entry_email); //API available from libgit2 v0.23.0
#endif
  git_config_free(gitconfig);

  git_commit * parents;
  unsigned int nb_parents = 0;
  if (!git_repository_head_unborn(repo))
  {
    git_oid head_oid;
    chk( git_reference_name_to_id(&head_oid, repo, "HEAD"), "Error: could not get HEAD oid\n" );
    chk( git_commit_lookup(&parents, repo, &head_oid), "Error: could not get HEAD commit\n" );
    nb_parents = 1;
  }

  git_tree * tree;
  git_oid tree_id;
  chk( git_index_write_tree(&tree_id, idx), "Error: could not get tree id\n" );
  chk( git_tree_lookup(&tree, repo, &tree_id), "Error: could not get tree\n" );

  git_oid new_commit_id;
  chk( git_commit_create(&new_commit_id, repo, "HEAD", me, me, "UTF-8", file_path, tree, nb_parents, (const git_commit **) &parents),
      "Error: could not create the commit\n" );

  // free all the remaining
  git_signature_free(me);
  git_commit_free(parents);
  git_tree_free(tree);
  free(abs_path);
  free(in_git_path);
  git_index_free(idx);
  git_repository_free(repo);
#if LIBGIT2_VER_MAJOR <= 0 && LIBGIT2_VER_MINOR < 22
  git_threads_shutdown();
#else
  git_libgit2_shutdown();
#endif
  free(git_repo_path);
  return 0;
}

// restore a file, copying it from the sysgeep repo to the host system
int sysgeep_restore(char * file_path, int sflag)
{
  fprintf(stderr, "Error: sysgeep_restore not yet implemented\n");
  abort();
  return 0;
}
