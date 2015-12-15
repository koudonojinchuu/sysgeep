#include <stdio.h> // printf()
#include <stdlib.h>// abort(), realloc()
#include <string.h>// strlen()

#include "utils.h"

// the first line must be the lines count (that's a "counted file")
static char ** counted_file_to_lines_array(char * file_path)
{
   FILE * fd = fopen(file_path, "r");
   pchk_t( fd, "Error, could not load lines from %s\n", file_path );

   // the first line is the number of lines
   char * line = NULL;
   size_t n = 0;
   getline(&line, &n, fd);
   int nb_lines = atoi(line);

   // allocate array of line pointers
   char ** lines;
   lines = malloc(sizeof(char *)*nb_lines);

   // load all other lines in an array
   free(line);
   line = NULL;
   int line_count = 0;
   while (getline(&line, &n, fd) != -1)
   {
      lines[line_count] = line;
      line = NULL;
      line_count++;
   }
   fclose(fd);

   return lines;
}

// to write down a lines array into a "counted file"
static void lines_array_to_counted_file(char ** lines, counted_file_path)
{
   int len = sizeof(lines) / sizeof(char *);
   FILE * fd = fopen(counted_file_path, "w+");
   fprintf(fd, "%d\n", len);
   int i;
   for (i=0; i<len; i++) fprintf(fd, "%s\n", lines[i]);
   close(fd);
}

static void free_lines_array(char ** lines_array)
{
   int i;
   int len = sizeof(lines_array) / sizeof(char *);
   for (i=0; i<len; i++) free(lines_array[i]);
   free(lines_array);
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

static int helper__index_of_closest_predecessor(char ** lines_array, char * str_to_lookup, bool * found, int beg, int end)
{
   if (beg > end) return -1;
   int mid = (end + beg) / 2;
   
   // if str_to_lookup == lines_array[mid], return mid
   // if str_to_lookup > lines_array[mid], return helper__(mid + 1, end)
   // if str_to_lookup < lines_array[mid], return helper__(beg, mid - 1)
   int comp = comparison_function(str_to_lookup, lines_array[mid]);
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
static int index_of_closest_predecessor(char ** lines_array, char * str_to_lookup, bool * found)
{
   int len = sizeof(lines_array) / sizeof(char *);
   if (!len) return -1;
   return helper__index_of_closest_predecessor(lines_array, str_to_lookup, found, 0, len - 1);
}

//XXX
void add_sorted_line(char * sysgeep_index_path, char * attributes_buffer)
{
   // load all lines into an array
   char ** lines = counted_file_to_lines_array(sysgeep_index_path);

   // search for the index where to insert
   bool found = 0;
   int pred_index = index_of_closest_predecessor(lines, attributes_buffer, &found);
   //TODO: if the line already exists, replace it.

   // insert
   char ** new_array = malloc(sizeof(char *)*(len + 1));
   int i;
   for (i=0; i<=pred_index; i++) new_array[i] = lines[i];
   new_array[1+pred_index] = attributes_buffer;
   for (i=pred_index+2; i<len+1; i++) new_array[i] = lines[i-1];

   // write back to a new file on disk
   char * new_index_path = malloc(sizeof(char)*(strlen(sysgeep_index_path) + 1 + 4));
   sprintf(new_index_path, "%s.new", sysgeep_index_path);
   lines_array_to_counted_file(lines, new_index_path);

   // free array
   free_lines_array(lines);

   // replace old index by the new
   unlink(sysgeep_index_path);
   rename(new_index_path, sysgeep_index_path);
   free(new_index_path);
}

void lookup_sorted_line(char * sysgeep_index_path, char * file_path)
{
   printf("lookup not yet implemented\n");
   abort();
}
