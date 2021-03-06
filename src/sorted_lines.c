#include <stdio.h>  // printf()
#include <stdlib.h> // abort()
#include <string.h> // strlen()
#include <unistd.h> // unlink()
#include <sys/stat.h>// stat()

#include "utils.h" // chk(), chk_t(), pchk_t()

// in the code, ** SPECIFIC ** means it is not generic of a "sorted lines" library

// the difference VS a raw char ** is that length can be 0
typedef struct struct_lines_array {
  char ** array;
  int length;
  char ** ends_of_keys; // stores where (index) would be '\0' if each line was shortened to its leading pathname
} s_lines_array;

static s_lines_array * create_lines_array(int length)
{
  s_lines_array * array_obj = malloc(sizeof(s_lines_array));
  array_obj->length = length;
  array_obj->array = malloc(sizeof(char *)*length);
  array_obj->ends_of_keys = malloc(sizeof(char *)*length);
  return array_obj;
}

static void free_lines_array(s_lines_array * lines_array)
{
  int i;
  int len = lines_array->length;
  for (i=0; i<len; i++) free(lines_array->array[i]);
  free(lines_array->array);
  free(lines_array->ends_of_keys);
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

// get the end of the pathname on the given line (where would be '\0'), before the attributes
// ** SPECIFIC **
static char * get_key_end(char * line)
{
  // go to the last character of 'user:group'
  char * result = line + strlen(line) - 6 - 2; // PERMLEN==6
  // find the space before 'user:group'
  while (*result != ' ') --result;
  return result;
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
    lines->ends_of_keys[line_count] = get_key_end(line);
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

// lexicographical order
// Compare only up to end1 and end2 (behave like they point to a '\0')
// to compare up to the "natural" end of the string, put NULL instead of the interested endx
// return -1 if line1 > line2
// return  1 if line1 < line2
// return  0 if line1 = line2
// ** SPECIFIC **
static int comparison_function(char * line1, char * line2, char * end1, char * end2)
{
  char * ptr1 = line1;
  char * ptr2 = line2;
  if (!end1) end1 = line1 + strlen(line1);
  if (!end2) end2 = line2 + strlen(line2);
  while (ptr1 != end1 && ptr2 != end2 && *(ptr1++) == *(ptr2++));
  // equality
  if (ptr1 == end1 && ptr2 == end2) return 0;
  // one line is prefix of the other
  if (ptr1 == end1) return 1;
  if (ptr2 == end2) return -1;
  // other difference
  --ptr1;
  --ptr2;
  if (*ptr1 > *ptr2) return -1;
  if (*ptr1 < *ptr2) return 1;
  // error
  fprintf(stderr, "Error: abnormal comparison\n");
  return -2;
}

static int helper__index_of_closest_predecessor(s_lines_array * lines_array, char * str_to_lookup, int * found, int beg, int end)
{
  if (beg > end) return -1;
  int mid = (end + beg) / 2;

  // if str_to_lookup == lines_array[mid], return mid
  // if str_to_lookup > lines_array[mid], return helper__(mid + 1, end)
  // if str_to_lookup < lines_array[mid], return helper__(beg, mid - 1)
  int comp = comparison_function(str_to_lookup, lines_array->array[mid], NULL, lines_array->ends_of_keys[mid]);
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
  return helper__index_of_closest_predecessor(lines_array, str_to_lookup, found, 0, len - 1);
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
