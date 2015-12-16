// helper function that exits with an error if the 'result' argument is not 0
void chk(int result, char * error_msg, ...);

// helper function that does the same but for "true" values instead of 0
void chk_t(int result, char * error_msg, ...);

// helper function that exits with an error if the 'result' argument is not NULL
void _pchk(void * result, char * error_msg, ...);

// helper function that exits with an error if the 'result' is NULL
void _pchk_t(void * result, char * error_msg, ...);

#define pchk(x, ...) _pchk( (void *) x, __VA_ARGS__ )
#define pchk_t(x, ...) _pchk_t( (void *) x, __VA_ARGS__ )
