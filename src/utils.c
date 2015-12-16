#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> // va_list, va_start(), va_end()

// helper function that exits with an error if the 'result' argument is not 0
void chk(int result, char * error_msg, ...)
{
  va_list args;
  va_start(args, error_msg);
  if (result)
  {
    vfprintf(stderr, error_msg, args);
    exit(1);
  }
  va_end(args);
}

// helper function that does the same but for "true" values instead of 0
void chk_t(int result, char * error_msg, ...)
{
  va_list args;
  va_start(args, error_msg);
  if (!result)
  {
    vfprintf(stderr, error_msg, args);
    exit(1);
  }
  va_end(args);
}

// helper function that exits with an error if the 'result' argument is not NULL
void _pchk(void * result, char * error_msg, ...)
{
  va_list args;
  va_start(args, error_msg);
  if (result)
  {
    vfprintf(stderr, error_msg, args);
    exit(1);
  }
  va_end(args);
}

// helper function that exits with an error if the 'result' is NULL
void _pchk_t(void * result, char * error_msg, ...)
{
  va_list args;
  va_start(args, error_msg);
  if (!result)
  {
    vfprintf(stderr, error_msg, args);
    exit(1);
  }
  va_end(args);
}
