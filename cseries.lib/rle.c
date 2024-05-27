/*
RLE.C
Sunday, December 15, 1991 12:06:24 AM

first long word is raw size
run count 0≤n<128 repeat n+3 of next byte
        128≤n≤255 n-127 bytes of raw data follows

Monday, March 9, 1992 12:22:43 PM
	fix to compress_bytes().
Monday, November 9, 1992 3:01:45 PM
	moved into cseries.lib, retrofitting a minotaur fix to uncompress bytes.  remember
	we still have that lurking problem where the size of the data before compression is
	sometimes not equal to the size of the data after compression and decompression.
Thursday, January 21, 1993 9:52:03 AM
	changed two ‘raw_size>=0’ expressions to ‘raw_size>0’ in compress_bytes, in an attempt to
	fix the above problem.  i doubt this is it.
Friday, February 19, 1993 1:58:23 PM
	the read pointer in compress bytes wasn’t getting incremented while scanning non-repeating
	data.  who the fuck did that and have they gotten the tree out of their ass yet?  and what
	the hell, the raw_count was not being reset after we’d scanned 127 (or whatever) bytes of
	uncompressable data.  blow me.  is it possible that minotaur’s shapes never contained
	that much uncompressable data?
Wednesday, March 3, 1993 8:29:03 PM
	this is a fucking comedy, right?  we were allowing repeats of 131 bytes, which encoded
	to [128] [byte] and only dropped one.  deja vu: “is it possible that minotaur’s shapes
	never contained that much COMPRESSABLE data?”  what a piece of shit.  that delayed me
	three hours.
*/

#include "cseries.h"
#include "rle.h"

#ifdef mpwc
#pragma segment modules
#endif

/* ---------- code */

long compress_bytes(
	byte *raw,
	long raw_size,
	byte *compressed,
	long maximum_compressed_size)
{
	register byte *read;
	register byte *write;
	byte *last_raw_count;
	long size;
	short count;
	register byte value;
	
	*((long*)compressed)= raw_size;
	read= raw, write= compressed+4;
	count= 0, size= 4, last_raw_count= (byte *) NULL;
	
	while (raw_size>0&&size<maximum_compressed_size)
	{
		value= *read;
		if (raw_size>=3&&value==*(read+1)&&value==*(read+2))
		{
			if (last_raw_count)
			{
				*last_raw_count= count+127;
			}
			
			count= 3;
			read+= 3;
			raw_size-= 3;
			while (raw_size>0&&count<130&&value==*read)
			{
				read+= 1;
				raw_size-= 1;
				count+= 1;
			}
			
			*write++= count-3;
			*write++= value;
			size+= 2;
			
			last_raw_count= (byte *) NULL;
			count= 0;
		}
		else
		{
			if (!last_raw_count)
			{
				last_raw_count= write++;
				size+= 1;
			}
			
			*write++= value;
			raw_size-= 1;
			count+= 1;
			size+= 1;
			
			if (count==255-127||!raw_size)
			{
				*last_raw_count= count+127;
				
				last_raw_count= (byte *) NULL;
				count= 0;
			}
			
			read+= 1;
		}
	}
	
	if (size>=maximum_compressed_size)
	{
		return -1;
	}
	else
	{
		return size;
	}
}

long get_destination_size(
	byte *compressed)
{
	return *((long*)compressed);
}

/* raw damn well have better been created after a get_destination_size call */
void uncompress_bytes(
	byte *compressed,
	byte *raw)
{
	register byte *read;
	register byte *write;
	long current_size, raw_size;
	short count;
	byte value;
	
	current_size= 0;
	raw_size= *((long*)compressed);
	read= compressed+sizeof(long), write= raw;
	
	while (current_size<raw_size)
	{
		count= *read++;
		if (count<128)
		{
			count+= 3;
			value= *read++;
			current_size+= count;
			assert(current_size<=raw_size);
			
			while (count)
			{
				*write++= value;
				count-= 1;
			}
		}
		else
		{
			count-= 127;
			current_size+= count;
			assert(current_size<=raw_size);
			
			while (count)
			{
				*write++= *read++;
				count-= 1;
			}
		}
	}
	assert(current_size==raw_size);
	
	return;
}
