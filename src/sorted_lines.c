#include <stdio.h>  // printf()
#include <stdlib.h> // abort()
#include <string.h> // strlen()
#include <unistd.h> // unlink()
#include <sys/stat.h>// stat()

#include "utils.h" // chk(), chk_t(), pchk_t()

// the difference VS a raw char ** is that length can be 0
typedef struct struct_lines_array {
  char ** array;
  int length;
} s_lines_array;

static s_lines_array * create_lines_array(int length)
{
  s_lines_array * array_obj = malloc(sizeof(s_lines_array));
  array_obj->length = length;
  array_obj->array = malloc(sizeof(char *)*length);
  return array_obj;
}

static void free_lines_array(s_lines_array * lines_array)
{
  int i;
  int len = lines_array->length;
  for (i=0; i<len; i++) free(lines_array->array[i]);
  free(lines_array->array);
  free(lines_array);
}

void init_counted_file(char * counted_file_path)
{
  // do nothing if the counted file already exists
  // else, create it with only one line containing '0'
  struct stat s;
  if (stat(counted_file_path, &s))
  {
    FILE * counted_file_fd;
    counted_file_fd = fopen(counted_file_path, "w+");
    fwrite("0", 1, 1, counted_file_fd);
    fclose(counted_file_fd);
  }
  else
  {
    chk_t(S_ISREG(s.st_mode), "Error: the counted file %s already exists but is not a regular file\n", counted_file_path);
    // if already exists and is a regular file, do nothing
  }
}

// the first line must be the lines count (that's a "counted file")
static s_lines_array * counted_file_to_lines_array(char * file_path)
{
  FILE * fd = fopen(file_path, "r");
  pchk_t( fd, "Error, could not load lines from %s\n", file_path );

  // the first line is the number of lines
  char * line = NULL;
  size_t n = 0;
  getline(&line, &n, fd);
  int nb_lines = atoi(line);

  // allocate array of line pointers
  s_lines_array * lines = create_lines_array(nb_lines);

  // load all other lines in an array
  free(line);
  line = NULL;
  int line_count = 0;
  while (getline(&line, &n, fd) != -1)
  {
    int line_end = strlen(line) - 1;
    if ((line_end >= 0) && (line[line_end] == '\n')) line[line_end]='\0';
    lines->array[line_count] = line;
    line = NULL;
    line_count++;
  }
  fclose(fd);

  return lines;
}

// to write down a lines array into a "counted file"
static void lines_array_to_counted_file(s_lines_array * lines, char * counted_file_path)
{
  int len = lines->length;
  FILE * fd = fopen(counted_file_path, "w+");
  fprintf(fd, "%d\n", len);
  int i;
  for (i=0; i<len; i++) fprintf(fd, "%s\n", lines->array[i]);
  fclose(fd);
}

// return the length of the line truncated to the first word (the key, i.e. the pathname)
static int get_length_key(char * line)
{
  // the line is of the form:
  // /usr/my/path/to/file user:group 100644
  // we want to output only the length of /usr/my/path/to/file
  int len = strlen(line);
  int cursor = len - 8;
  while (line[cursor] != ' ') cursor--;
  return cursor;
}

// lexicographical order
// return -1 if line1 > line2
// return  1 if line1 < line2
// return  0 if line1 = line2
static int comparison_function(char * line1, char * line2)
{
  int len1 = get_length_key(line1);
  int len2 = get_length_key(line2);

  // degenerate cases
  if ((len1 == 0) && (len2 == 0)) return 0;
  if (!len1) return 1;
  if (!len2) return -1;

  // main cases
  int minlen = (len1 <= len2) ? len1 : len2;
  int i;
  for (i=0; i<minlen; i++)
  {
    if (line1[i] > line2[i]) return -1;
    if (line1[i] < line2[i]) return 1;
  }
  // one key is prefix of the other
  if (len1 > len2) return -1;
  if (len1 < len2) return 1;

  return 0;
}

static int helper__index_of_closest_predecessor(s_lines_array * lines_array, char * str_to_lookup, int * found, int beg, int end)
{
  if (beg > end) return -1;
  int mid = (end + beg) / 2;

  // if str_to_lookup == lines_array[mid], return mid
  // if str_to_lookup > lines_array[mid], return helper__(mid + 1, end)
  // if str_to_lookup < lines_array[mid], return helper__(beg, mid - 1)
  int comp = comparison_function(str_to_lookup, lines_array->array[mid]);
  if (comp == 0)
  {
    *found = 1;
    return mid;
  }
  if (comp == -1)
  {
    if (end == beg) { *found = 0; return mid; }
    return helper__index_of_closest_predecessor(lines_array, str_to_lookup, found, mid + 1, end);
  }
  if (comp == 1)
  {
    if (end == beg) { *found = 0; return mid - 1; }
    return helper__index_of_closest_predecessor(lines_array, str_to_lookup, found, beg, mid - 1);
  }
  abort();
}

// If some line has exactly that name, will return its index.
// Else, return the line that comes just before it (its lower bound).
// If there are no lines in the array, return -1
static int index_of_closest_predecessor(s_lines_array * lines_array, char * str_to_lookup, int * found)
{
  int len = lines_array->length;
  if (!len) return -1;
  int result = helper__index_of_closest_predecessor(lines_array, str_to_lookup, found, 0, len - 1);
  // check that it is not just a keyword to whom the searched keyword is a prefix
  if (*found)
  {
    char after_keyword = lines_array->array[result][strlen(str_to_lookup)];
    int isprefix = (after_keyword == ' ' || after_keyword == '\0');
    if (isprefix)
    {
      *found = 0;
      // decrement the predecessor index
      // the line was allegedly found, so result >= 0
      --result;
    }
  }
  return result;
}

// add a line in the sorted index, or replace the line if the same key preexisted
void add_sorted_line(char * index_path, char * attributes_buffer)
{
  // load all lines into an array
  s_lines_array * lines = counted_file_to_lines_array(index_path);
  int len = lines->length;

  // search for the index where to insert
  int found = 0;
  int pred_index = index_of_closest_predecessor(lines, attributes_buffer, &found);

  // insert (or replace if found==1)
  s_lines_array * new_array = create_lines_array(len + !found);
  int i;
  for (i=0; i<=pred_index; i++) new_array->array[i] = lines->array[i];
  new_array->array[!found+pred_index] = strdup(attributes_buffer);
  for (i=pred_index+1+!found; i<len+!found; i++) new_array->array[i] = lines->array[i-!found];

  // the array "lines" gave its elements to "new_array" so free only the remaining
  free(lines->array);
  free(lines);

  // write back to a new file on disk
  char * new_index_path = malloc(sizeof(char)*(strlen(index_path) + 1 + 4));
  sprintf(new_index_path, "%s.new", index_path);
  lines_array_to_counted_file(new_array, new_index_path);

  // free arrays
  free_lines_array(new_array);

  // replace old index by the new
  unlink(index_path);
  rename(new_index_path, index_path);
  free(new_index_path);
}

// return the line corresponding to str_to_lookup in index_path
// return NULL if not found
char * lookup_sorted_line(char * index_path, char * str_to_lookup)
{
  // load all lines into an array
  s_lines_array * lines = counted_file_to_lines_array(index_path);
  int len = lines->length;

  // lookup for the line's index
  int found = 0;
  int pred_index = index_of_closest_predecessor(lines, str_to_lookup, &found);

  if (found)
  {
    char * whole_line_found = strdup(lines->array[pred_index]);
    free_lines_array(lines);
    return whole_line_found;
  }
  return NULL;
}
