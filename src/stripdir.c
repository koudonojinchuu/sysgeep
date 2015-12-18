// the same behaviour as the commad "realpath -s"
char *realpath_s(char * pathname) {
	int ldots;
	ldots = 0;

	//in   = dir;
	//out  = buf;
	//last = buf + maxlen;
	//*out  = 0;

	char * result;
  // if pathname is relative, use getcwd()
	if (pathname[0] != '/') {
    // error if cwd cannot be known
    char * cwd;
		if ( !(cwd = getcwd(NULL, 0)) ) return NULL;

    // make sure that the last char of cwd is '/'
    int cwd_len = strlen(cwd);
    //out = buf + strlen(buf) - 1;
    if (cwd[cwd_len - 1] != '/')
    {
      char * cwd = realloc(cwd, sizeof(char*)*(2+cwd_len));
      sprintf(cwd, "%s/", cwd);
    }
    //out++;
	}

	while (out < last) {
		*out = *in;

    // if we are on '/', go ahead, then if again on a '/', go ahead,
    // until the current char is not '/', then go to immediately  previous '/'
		if (*in == '/')
		{
			while (*(++in) == '/') ;
			in--;
		}

    // if in[0]=='\0' or if on a '/'
		if (*in == '/' || !*in)
		{
      // go backwards and reduce .. or . occurrences
			if (ldots == 1 || ldots == 2) {
				while (ldots > 0 && --out > cwd) if (*out == '/') ldots--;
        out[1]='\0'; // terminates temporarily the result string
			}
			ldots = 0;

		} else if (*in == '.' && ldots > -1) {
			ldots++;
		} else {
			ldots = -1;
		}

		out++;

		if (!*in)
			break;

		in++;
	}

	if (*in) {
		errno = ENOMEM;
		return NULL;
	}

	while (--out != buf && (*out == '/' || !*out)) *out=0;
	return buf;
}

