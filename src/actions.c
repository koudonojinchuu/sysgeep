#include <stdlib.h> // getenv(), abort(), realpath()
#include <stdio.h>  // fprintf()
#include <string.h> // strlen()
#include <sys/stat.h>// stat(), S_ISREG(), chmod()
#include <error.h>  // error()
#include <unistd.h> // access(), chown()
#include <libgen.h> // dirname(), basename()
#include <git2.h>   // use git without system() calls
#include <fcntl.h>  // O_RDONLY, O_WRONLY, O_CREAT
#include <assert.h> // assert()

#include "utils.h"
#include "sorted_lines.h"
#include "stripdir.h"

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
  int need_to_free_path = 1;

  if (sflag)
  {
    config_file_path = ROOT_CONFIG_LOCATION;
    need_to_free_path = 0;
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
      chk( access(config_file_path, R_OK),
        "Error: cannot read in %s\nMaybe you want to create it with (with root permissions): sysgeep -s setup <path/to/sysgeep/repo>\n",
          config_file_path );
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
  if (need_to_free_path) free(config_file_path);
  return config_file_path_fd;
}

static char * get_sysgeep_index(char * git_repo_path)
{
  char * sysgeep_index_path = malloc(sizeof(char)*(strlen(git_repo_path) + strlen("/.sysgeep_index") + 1));
  sprintf(sysgeep_index_path, "%s/.sysgeep_index", git_repo_path);
  return sysgeep_index_path;
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

int sysgeep_print_setup(int sflag)
{
  char * git_repo = get_git_repo(sflag);
  chk(!git_repo || !strcmp(git_repo, ""), "Error: the sysgeep repository is not yet set. Use 'sysgeep [-s] setup <repo path>'.\n");
  printf("sysgeep repository path: %s\n", git_repo);
  free(git_repo);
  return 0;
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
  char * sysgeep_index_path = get_sysgeep_index(local_git_repo_path);
  init_counted_file(sysgeep_index_path);
  free(sysgeep_index_path);

  // copy the git repo name into the config file
  FILE * config_file_fd = config_file(sflag, 'w');
  fwrite(local_git_repo_path, 1, sizeof(char)*strlen(local_git_repo_path), config_file_fd);

  fclose(config_file_fd);
  return 0;
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

  char * sysgeep_index_path = get_sysgeep_index(git_repo_path);

  // get file (or dir) attributes and build line to put into the sysgeep index
  struct stat s;
  chk( stat(abs_path_dup, &s), "Error: could not stat() the file or directory to be saved\n" );
  char * attributes_buffer = malloc(sizeof(char)*(strlen(abs_path_dup) + UIDT_MAXLEN + GIDT_MAXLEN + PERM_LEN + 4));
  sprintf(attributes_buffer, "%s %d:%d %06o", abs_path_dup, s.st_uid, s.st_gid, s.st_mode);

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

// create or update the file copied from "src_path" to "dest_path" in the git repo
static void copy_file(char * src_path, char * dest_path)
{
  FILE * file_to_write = fopen(dest_path, "w");
  FILE * src = fopen(src_path, "r");
  char c;
  while( (c = fgetc(src)) != EOF ) fputc(c, file_to_write); 
  fclose(src);
  fclose(file_to_write);
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
  copy_file(file_path, in_git_path);

  // take note of the permissions and owner:group
  // keep them on a line in a lexicographic ordered file at root of the git repo
  add_to_index(git_repo_path, abs_path);

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
  // add the .sysgeep_index too
  chk( git_index_add_bypath(idx, ".sysgeep_index"), "Error: could not add object to the index\n" );
  chk( git_index_write(idx), "Error: could not write index to disk\n" );

  // commit with the file name as message
  git_config * gitconfig = NULL;
  chk( git_config_open_default(&gitconfig), "Error: could not open git configuration\n" );
#if LIBGIT2_VER_MAJOR >= 0 && LIBGIT2_VER_MINOR >= 23
  git_config_entry * entry_name;
  chk( git_config_get_entry(&entry_name, gitconfig, "user.name"), "Error: could not get user.name entry\n" );
  git_config_entry * entry_email;
  chk( git_config_get_entry(&entry_email, gitconfig, "user.email"), "Error: could not get user.email entry\n" );
#else
  git_config_entry * entry_name;
  chk( git_config_get_entry((const git_config_entry **) &entry_name, gitconfig, "user.name"), "Error: could not get user.name entry\n" );
  git_config_entry * entry_email;
  chk( git_config_get_entry((const git_config_entry **) &entry_email, gitconfig, "user.email"), "Error: could not get user.email entry\n" );
#endif
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

// take an absolute pathname and transform it to the corresponding absolute
// pathname of the file in sysgeep repository
static char * path_from_sysgeep_of(char * abs_path, char * git_repo_path)
{
  char * in_git_path = malloc(sizeof(char)*(strlen(abs_path) + strlen(git_repo_path) + 1));
  sprintf(in_git_path, "%s%s", git_repo_path, abs_path);
  return in_git_path;
}

// restore a file, copying it from the sysgeep repo to the host system
// the argument is the path in the system, not the one in the sysgeep repo
int sysgeep_restore(char * file_path, int sflag)
{
  char * git_repo_path = get_git_repo(sflag);

  // get absolute path of "file_path"
  char * abs_path = realpath_s(file_path); // like command line's "realpath -s"
  pchk_t( abs_path, "Error: could not find the real path of: %s\n", file_path );

  // make it a path inside the git repo
  char * in_git_path = path_from_sysgeep_of(abs_path, git_repo_path);

  // if it is not in sysgeep_index, error.
  char * sysgeep_index_path = get_sysgeep_index(git_repo_path);
  char * attributes = lookup_sorted_line(sysgeep_index_path, abs_path);
  free(sysgeep_index_path);
  pchk_t( attributes, "Error: could not find file in sysgeep_index: %s\n", abs_path );
  char * endptr = attributes + strlen(abs_path) + 1; // go after the keyword and its trailing space
  int user = strtol(endptr, &endptr, 10);
  ++endptr;
  int group = strtol(endptr, &endptr, 10);
  ++endptr;
  int modes = strtol(endptr, &endptr, 8);
  free(attributes);

  struct stat s;
  // check whether it is a directory according to .sysgeep_index infos
  if (S_ISREG(modes)) // if regular file, copy
  {
    // cp from inside the git repo to the absolute path on the system
    int in_fd = open(in_git_path, O_RDONLY);
    assert(in_fd >= 0);
    int out_fd = open(abs_path, O_WRONLY | O_CREAT);
    assert(out_fd >= 0);
    char buf[8192];
    ssize_t result;
    while ( ( result = read(in_fd, &buf[0], sizeof(buf)) ) )
      assert(write(out_fd, &buf[0], result) == result);
  }
  else if (S_ISDIR(modes) && stat(file_path, &s)) // if directory, make it when absent
  {
    mode_t old_mask = umask(0);
    chk( mkdir(file_path, modes), "Error: could not create directory %s\n", file_path );
    umask(old_mask);
  }
  else if (stat(file_path, &s)) // if not a directory but file is absent, just copy it (permissions will be set anyway)
  {
    copy_file(in_git_path, file_path);
  }

  //TODO: system to only set attributes when they actually differ
  //      (and be able to choose to touch the file anyway)
  // set permissions, owner and group
  chk( chmod(file_path, modes), "Error: could not set permissions of: %s\n", file_path );
  chk( chown(file_path, user, group), "Error: could not set owner or group of: %s\n", file_path );

  //printf("Found file: permissions: %o\n", modes);

  free(abs_path);
  free(in_git_path);

  return 0;
}

int sysgeep_setup_remote(char * remote_repo_url, int sflag)
{
  char * sysgeep_repo = get_git_repo(sflag); 
  sysgeep_setup_remote_for_repo(remote_repo_url, sysgeep_repo, sflag);
  free(sysgeep_repo);
  return 0;
}

int sysgeep_setup_remote_for_repo(char * remote_repo_url, char * local_git_repo_path, int sflag);
{
  git_repository * repo = NULL;
  chk( git_repository_open(&repo, git_repo_path), "Error: could not open git repository: %s\n", git_repo_path );
  git_remote_delete(repo, "sysgeep_origin");
  git_remote * newremote = NULL;
  chk( git_remote_create(&newremote, repo, "sysgeep_origin", remote_repo_url),
    "Error: could not add remote %s to sysgeep repository\n", remote_repo_url );
  git_remote_free(repo);
  git_repository_free(repo);
  return 0;
{

int sysgeep_push(int sflag)
{
  fprintf(stderr, "Not yet implemented.\n");
  return 0;
}

int sysgeep_push(int sflag)
{
  fprintf(stderr, "Not yet implemented.\n");
  return 0;
}
