#include "stripdir.c"

int main(int argc, char ** argv)
{
  if (argc > 1)
    printf("%s\n",realpath_s(argv[1]));
  return 0;
}
