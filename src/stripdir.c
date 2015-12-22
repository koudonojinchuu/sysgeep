// the same behaviour as the command "realpath -s"
char *realpath_s(char * pathname) {

	char * result;
  char * cwd = NULL;
  int cwd_len = 0;
  int cwd_needs_trailing_slash = 0;
  // if pathname is relative, use getcwd()
	if (pathname[0] != '/') {
    // error if cwd cannot be known
		if ( !(cwd = getcwd(NULL, 0)) ) return NULL;
    // make sure that cwd will be terminated by '/'
    cwd_len = strlen(cwd);
    if (cwd[cwd_len - 1] != '/') cwd_needs_trailing_slash = 1;
	}

  // the maximum required size for the buffer will be 
  // len_cwd + len_pathname + 2
  int pathname_len = strlen(pathname);
  int buf_len = cwd_len + pathname_len + 1;
  char * buf_ptr;
  char * buf = malloc(sizeof(char)*(buf_len + 1));
  char * buf_last = buf + buf_len; // pointer to the last usable character of buf (included terminating '\0')
  // after that, buf_ptr points to the character just after cwd/
  if (cwd_needs_trailing_slash)
  {
    sprintf(buf, "%s/", cwd);
    buf_ptr = buf + cwd_len + 1;
  }
  else if (cwd)
  {
    sprintf(buf, "%s", cwd);
    buf_ptr = buf + cwd_len;
  }
  else
  {
    buf[0] = '\0';
    buf_ptr = buf;
  }

  char * pathname_ptr = pathname;
	int dots = 0;
	while (buf_ptr < buf_last) {
		*buf_ptr = *pathname_ptr; // copy chars from pathname to current buf_ptr

    // if we are on '/', go ahead, then if again on a '/', go ahead,
    // until the current char is not '/', then go to immediately  previous '/'
		if (*pathname_ptr == '/')
		{
			while (*(++pathname_ptr) == '/') ;
			pathname_ptr--;
		}

    // if pathname_ptr[0]=='\0' or if on a '/'
		if (*pathname_ptr == '/' || !*pathname_ptr)
		{
      // we reached a slash or the end, so go backwards and reduce the last recorded .. or . occurrence
			if (dots == 1 || dots == 2) {
				while (dots > 0 && --buf_ptr > cwd) if (*buf_ptr == '/') dots--;
        buf_ptr[1]='\0'; // terminates temporarily the result string
			}
      // cancel any 'event' occurence, because we met a slash (or are at the begining of the string)
			dots = 0;

    // if 'event' did not occur, and we are on a dot, count it
		} else if (*pathname_ptr == '.' && dots > -1) {
			dots++;
		} else { // 'event' occurs: we reached a non-dot, non-slash character
			dots = -1;
		}

    // increasing 'buf_ptr' pointer
		buf_ptr++;

    // if we reached the end of pathname (ie '\0' char), exit while loop
		if (!*pathname_ptr) break;

    // increasing pathname pointer
		pathname_ptr++;
	}

  // error if we reached "buf_last" but some characters of pathname are still left
	if (*pathname_ptr) {
		errno = ENOMEM;
		return NULL;
	}

  // replace all trailing slashes by null chars
	while (--buf_ptr != buf && (*buf_ptr == '/' || !*buf_ptr)) *buf_ptr=0;
	return buf;
}

