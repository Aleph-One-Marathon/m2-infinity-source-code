/* 
checksum.c
Thursday, March 10, 1994 8:21:23 PM
	written by ajr
*/

#include <stdlib.h>  /* for srand() and rand() */
#include <time.h>    /* for clock() */

#include "cseries.h"

#include "checksum.h"

// private prototypes
void update_add_checksum(Checksum *check, word *src, long length);

/**********************************************************************************
 *
 * Function: new_checksum
 * Purpose:  initialize the checksum data structure.
 *
 **********************************************************************************/
void new_checksum(Checksum *check, word type)
{
	// set up bogus stuff in the checksum, so when it's saved, it's a 
	// bit obfuscated.
	srand(clock());
	check->bogus1 = rand();
	check->bogus2 = rand();

	assert(type == ADD_CHECKSUM);

	check->checksum_type = type;
	
	switch (check->checksum_type)
	{
		case ADD_CHECKSUM:
			check->value.add_checksum = 0;
			break;
		case FLETCHER_CHECKSUM:
		case CRC32_CHECKSUM:
		default:
			halt();
			break;
	}
}

/**********************************************************************************
 *
 * Function: update_checksum
 * Purpose:  takes the given checksum and updates it for the extra stuff.
 *
 **********************************************************************************/
void update_checksum(Checksum *check, word *src, long length)
{
	switch (check->checksum_type)
	{
		case ADD_CHECKSUM:
			update_add_checksum(check, src, length);
			break;
		case FLETCHER_CHECKSUM:
		case CRC32_CHECKSUM:
		default:
			halt();
			break;
	}
}

/**********************************************************************************
 *
 * Function: update_add_checksum
 * Purpose:  called by update_checksum to take the given checksum and add all
 *           the stuff in src to it.
 *
 **********************************************************************************/
void update_add_checksum(Checksum *check, word *src, long length)
{
	long i;
	long real_length;

	if (length % 2) // is it odd?
		length--;   // make it even. ignore last byte.
	
	real_length = length / sizeof(word); // duh

	for (i = 0; i < real_length; i++)
	{
		check->value.add_checksum += *(src+i);
	}
}

/**********************************************************************************
 *
 * Function: equal_checksums
 * Purpose:  decide if 2 checksums are the same. wow.
 *
 **********************************************************************************/
boolean equal_checksums(Checksum *check1, Checksum *check2)
{
	assert(check1->checksum_type == check2->checksum_type);
	switch(check1->checksum_type)
	{
		case ADD_CHECKSUM:
			return (check1->value.add_checksum == check2->value.add_checksum);
			break;
		case FLETCHER_CHECKSUM:
		case CRC32_CHECKSUM:
		default:
			halt(); // not implemented yet.
			break;
	}
}