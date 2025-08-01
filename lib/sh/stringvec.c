/* stringvec.c - functions for managing arrays of strings. */

/* Copyright (C) 2000-2002,2022-2024 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <bashtypes.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <bashansi.h>
#include <stdckdint.h>
#include <stdio.h>
#include <chartypes.h>

#include "shell.h"

/* Allocate an array of strings with room for N members. */
char **
strvec_create (size_t n)
{
  return ((char **)xreallocarray (NULL, n, sizeof (char *)));
}

/* Allocate an array of strings with room for N members. */
char **
strvec_mcreate (size_t n)
{
  return ((char **)reallocarray (NULL, n, sizeof (char *)));
}

char **
strvec_resize (char **array, size_t nsize)
{
  return ((char **)xreallocarray (array, nsize, sizeof (char *)));
}

char **
strvec_mresize (char **array, size_t nsize)
{
  return ((char **)reallocarray (array, nsize, sizeof (char *)));
}

/* Return the length of ARRAY, a NULL terminated array of char *. */
size_t
strvec_len (char * const *array)
{
  register size_t i;

  for (i = 0; array[i]; i++);
  return (i);
}

/* Free the contents of ARRAY, a NULL terminated array of char *. */
void
strvec_flush (char **array)
{
  register size_t i;

  if (array == 0)
    return;

  for (i = 0; array[i]; i++)
    free (array[i]);
}

void
strvec_dispose (char **array)
{
  if (array == 0)
    return;

  strvec_flush (array);
  free (array);
}

int
strvec_remove (char **array, const char *name)
{
  size_t i, j;
  char *x;

  if (array == 0)
    return 0;

  for (i = 0; array[i]; i++)
    if (STREQ (name, array[i]))
      {
	x = array[i];
	for (j = i; array[j]; j++)
	  array[j] = array[j + 1];
	free (x);
	return 1;
      }
  return 0;
}

/* Find NAME in ARRAY.  Return the index of NAME, or -1 if not present.
   ARRAY should be NULL terminated. */
ptrdiff_t
strvec_search (char **array, const char *name)
{
  size_t i;

  for (i = 0; array[i]; i++)
    if (STREQ (name, array[i]))
      return (i);

  return (-1);
}

/* Allocate and return a new copy of ARRAY and its contents. */
char **
strvec_copy (char * const *array)
{
  size_t i, len;
  char **ret;

  len = strvec_len (array);

  ret = (char **)xmalloc ((len + 1) * sizeof (char *));
  for (i = 0; array[i]; i++)
    ret[i] = savestring (array[i]);
  ret[i] = (char *)NULL;

  return (ret);
}

/* Comparison routine for use by qsort that conforms to the new Posix
   requirements (http://austingroupbugs.net/view.php?id=1070).

   Perform a bytewise comparison if *S1 and *S2 collate equally. */
int
strvec_posixcmp (char **s1, char **s2)
{
  int result;

#if defined (HAVE_STRCOLL)
   result = strcoll (*s1, *s2);
   if (result != 0)
     return result;
#endif

  if ((result = **s1 - **s2) == 0)
    result = strcmp (*s1, *s2);

  return (result);
}

/* Comparison routine for use with qsort() on arrays of strings.  Uses
   strcoll(3) if available, otherwise it uses strcmp(3). */
int
strvec_strcmp (char **s1, char **s2)
{
#if defined (HAVE_STRCOLL)
   return (strcoll (*s1, *s2));
#else /* !HAVE_STRCOLL */
  int result;

  if ((result = **s1 - **s2) == 0)
    result = strcmp (*s1, *s2);

  return (result);
#endif /* !HAVE_STRCOLL */
}

/* Sort ARRAY, a null terminated array of pointers to strings. */
void
strvec_sort (char **array, int posix)
{
  if (posix)
    qsort (array, strvec_len (array), sizeof (char *), (QSFUNC *)strvec_posixcmp);
  else
    qsort (array, strvec_len (array), sizeof (char *), (QSFUNC *)strvec_strcmp);
}

/* Cons up a new array of words.  The words are taken from LIST,
   which is a WORD_LIST *.  If ALLOC is true, everything is malloc'ed,
   so you should free everything in this array when you are done.
   The array is NULL terminated.  If IP is non-null, it gets the
   number of words in the returned array.  STARTING_INDEX says where
   to start filling in the returned array; it can be used to reserve
   space at the beginning of the array. */

char **
strvec_from_word_list (WORD_LIST *list, int alloc, int starting_index, int *ip)
{
  size_t count;
  char **array;

  count = list_length ((GENERIC_LIST *)list);
  array = (char **)xmalloc ((1 + count + starting_index) * sizeof (char *));

  for (count = 0; count < starting_index; count++)
    array[count] = (char *)NULL;
  for (count = starting_index; list; count++, list = list->next)
    array[count] = alloc ? savestring (list->word->word) : list->word->word;
  array[count] = (char *)NULL;

  if (ip)
    *ip = count;
  return (array);
}

/* Convert an array of strings into the form used internally by the shell.
   ALLOC means to allocate new storage for each WORD_DESC in the returned
   list rather than copy the values in ARRAY.  STARTING_INDEX says where
   in ARRAY to begin. */

WORD_LIST *
strvec_to_word_list (char **array, int alloc, int starting_index)
{
  WORD_LIST *list;
  WORD_DESC *w;
  size_t i, count;

  if (array == 0 || array[0] == 0)
    return (WORD_LIST *)NULL;

  for (count = 0; array[count]; count++)
    ;

  for (i = starting_index, list = (WORD_LIST *)NULL; i < count; i++)
    {
      w = make_bare_word (alloc ? array[i] : "");
      if (alloc == 0)
	{
	  free (w->word);
	  w->word = array[i];
	}
      list = make_word_list (w, list);
    }
  return (REVERSE_LIST (list, WORD_LIST *));
}
