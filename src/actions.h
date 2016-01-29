int sysgeep_print_setup(int sflag);
int sysgeep_setup(char * local_git_repo_path, int sflag);
int sysgeep_setup_remote(char * remote_repo_url, int sflag);
int sysgeep_setup_remote_for_repo(char * remote_repo_url, char * local_git_repo_path, int sflag);
int sysgeep_save(char * file_path, int sflag);
int sysgeep_restore(char * file_path, int sflag);
int sysgeep_push(int sflag);
int sysgeep_pull(int sflag);
