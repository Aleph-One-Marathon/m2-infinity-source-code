/*
PROXIMITY_STRCMP.C
Wednesday, December 9, 1992 6:44:28 PM

Wednesday, December 9, 1992 6:57:28 PM
	pattern matcher from dr. dobb’s journal, ‘heavily modified’.
*/

#include <string.h>
#include <ctype.h>

#include "cseries.h"
#include "proximity_strcmp.h"

/* ---------- private prototypes */

short find_pattern(char *string1, short len1, char *string2, short len2);

/* ---------- code */

short proximity_strcmp(
	char *string1,
	char *string2)
{
	short total;

	total= find_pattern(string1, strlen(string1), string2, strlen(string2));

	return (2*PERFECT_MATCH*total)/(strlen(string1)+strlen(string2));
}

/* ---------- private code */

#if 0
short find_pattern(
	char *string1,
	short len1,
	char *string2,
	short len2)
{
	register short pos1, pos2, offset;
	register short current, longest, lpos1, lpos2;
	register short total;

	for (longest= pos1= 0;pos1<len1;++pos1)
	{
		for (pos2= 0;pos2<len2;++pos2)
			for (current= offset= 0;1;++offset, ++current)
		{
			if ((pos2+offset>=len2)||(pos1+offset>=len1)
				||*(string1+pos1+offset)!=*(string2+pos2+offset))
			{
				if (current>longest)
					longest= current, lpos1= pos1, lpos2= pos2;
				break;
			}
		}
	}
	
	if (!longest)
	{
		return 0;
	}

	total= longest;
	if (lpos1&&lpos2)
	{
		total+= find_pattern(string1, lpos1, string2, lpos2);
	}
	if ((lpos1+longest<len1)&&(lpos2+longest<len2))
	{
		total+= find_pattern(string1+lpos1+longest, len1-lpos1-longest,
			string2+lpos2+longest, len2-lpos2-longest);
	}

	return total;
}

short find_pattern(
	char *string1,
	short length1,
	char *string2,
	short length2)
{
	register short total, pos1, pos2, current;
	register char *ptr1, *ptr2;
	short longest, lpos1, lpos2;

	for (longest= pos1= 0;pos1<length1;++pos1)
	{
		for (pos2= 0;pos2<length2;++pos2)
		{
			for (ptr1= string1+pos1, ptr2= string2+pos2, current= 0;
				(pos2+current<length2) && (pos1+current<length1) && tolower(*ptr1)==tolower(*ptr2);
				current++, ptr1++, ptr2++)
			if (current>longest)
			{
				longest= current, lpos1= pos1, lpos2= pos2;
			}
		}
	}
	
	total= longest;
	if (total)
	{
		if (lpos1&&lpos2)
		{
			total+= find_pattern(string1, lpos1, string2, lpos2);
		}
		if ((lpos1+longest<length1)&&(lpos2+longest<length2))
		{
			total+= find_pattern(string1+lpos1+longest, length1-lpos1-longest,
				string2+lpos2+longest, length2-lpos2-longest);
		}
	}

	return total;
}
#endif

short find_pattern(
  char *string1,
  short len1,
  char *string2,
  short len2)
{
  short pos1, pos2, offset;
  short current, longest, lpos1, lpos2;
  short total;

  longest= 0;

  for (pos1=0;pos1<len1;++pos1)
     for (pos2=0;pos2<len2;++pos2)
        for (current=offset=0;1;++offset,++current)
        {
           if ((pos2+offset>=len2)||(pos1+offset>=len1)
              ||tolower(*(string1+pos1+offset))!=tolower(*(string2+pos2+offset)))
           {
              if (current>longest)
                 longest=current,lpos1=pos1,lpos2=pos2;
              break;
           }
        }

  if (!longest)
     return 0;

  total= longest;
  if (lpos1&&lpos2)
     total+= find_pattern(string1, lpos1, string2, lpos2);
  if ((lpos1+longest<len1)&&(lpos2+longest<len2))
     total+= find_pattern(string1+lpos1+longest, len1-lpos1-longest,
	     string2+lpos2+longest, len2-lpos2-longest);

  return (total);
}
