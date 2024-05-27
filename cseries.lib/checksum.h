#ifndef __CHECKSUM_H
#define __CHECKSUM_H

/* 
checksum.h
Thursday, March 10, 1994 8:06:07 PM
	written by ajr
*/

// To add extra checksum types, the following enum and struct need to be expanded.
enum
{
	ADD_CHECKSUM, 
	FLETCHER_CHECKSUM, 
	CRC32_CHECKSUM
};

#ifdef envppc
#pragma options align=mac68k
#endif
typedef struct
{
	long bogus1;        // meant for obfuscating saved resource
	word checksum_type; // one of enums above
	union {             // value of the checksum; which one depends on checksum_type
		word add_checksum;
		word fletcher_checksum;
		long crc32_checksum;
		} value;
	long bogus2;        // meant for obfuscating saved resource
} Checksum;
#ifdef envppc
#pragma options align=reset
#endif

// function prototypes
void new_checksum(Checksum *check, word type);
void update_checksum(Checksum *check, word *src, long length);
boolean equal_checksums(Checksum *check1, Checksum *check2);

#endif
