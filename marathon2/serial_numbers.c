/*
SERIAL_NUMBERS.C
Thursday, December 1, 1994 4:56:29 PM  (Jason)

Monday, December 5, 1994 1:12:01 PM  (Jason')
	heinous bug somewhere which rocks our stack frame (in ex libris, at the reg)
Thursday, December 8, 1994 4:03:13 PM  (Jason')
	blacklist.
*/

/*
serial numbers are ten bytes (eighty bits, sixteen tokens)
every other bit is redundant, and comes from the constant pad
every bit is XORed with the previous bit

1 data bit, 1 checksum bit ==> 4 codes (2 valid, 2 invalid)
2 data bits, 2 checksum bits ==> 16 codes (4 valid, 12 invalid)
etc...

the twenty valid bits are divided as follows:
[16 lot number]
[8 network-only bits]
[16 sequential serial number]
*/

#include "cseries.h"

#include <string.h>
#include <ctype.h>

/* ----------- constants */

#define BITS_PER_TOKEN 5
#define NUMBER_OF_TOKENS (1<<BITS_PER_TOKEN)
#define BITS_PER_SHORT_SERIAL_NUMBER 40
#define BITS_PER_LONG_SERIAL_NUMBER 80
#define BYTES_PER_SHORT_SERIAL_NUMBER (BITS_PER_SHORT_SERIAL_NUMBER/8)
#define BYTES_PER_LONG_SERIAL_NUMBER (BITS_PER_LONG_SERIAL_NUMBER/8)
#define TOKENS_PER_SERIAL_NUMBER 16

/* ----------- macros */

#define PADS_ARE_EQUAL(p1, p2) (*((long*)p1)==*((long*)p2) && *((long*)(p1+1))==*((long*)(p2+1)))
#define VALID_INVERSE_SEQUENCE(s) ((((char)s[2])>0) ? ((word)*((short*)(s+3))==calculate_complement(*((word*)(s)))) : \
	((word)(*((short*)(s+3))^0xffff)==calculate_complement(*((word*)s)^0xffff)))

#define EXTRACT_BIT(ptr, bit) ((((byte*)ptr)[(bit)>>3] & (1<<((bit)&7))) ? TRUE : FALSE)
#define INSERT_BIT(ptr, bit, value) if (value) \
	((byte*)ptr)[(bit)>>3]|= (1<<((bit)&7)); else \
	((byte*)ptr)[(bit)>>3]&= (byte)~(1<<((bit)&7))

/* ---------- globals */

static char raw_token_to_ascii_token[NUMBER_OF_TOKENS]=
{
	'P', '9', 'L', 'K', 'Y', '1', '3', 'Q', 'J',
	'M', 'N', '8', 'V', 'G', 'A', 'U', 'H', 'B',
	'4', 'C', 'X', 'D', 'F', '5', 'T', 'R', '2',
	'E', 'S', '7', '6', 'Z'
};

//static byte actual_pad[BITS_PER_SHORT_SERIAL_NUMBER/8]= {0xa5, 0x79, 0x4e, 0xf3, 0x9a}; // marathon1
//static byte actual_pad[BITS_PER_SHORT_SERIAL_NUMBER/8]= {0x55, 0x19, 0x9f, 0xaa}; // marathon2
//static byte actual_pad[BITS_PER_SHORT_SERIAL_NUMBER/8]= {0x77, 0x68, 0x6f, 0x72, 0x65}; // marathon infinity betas
static byte actual_pad_m2[BITS_PER_SHORT_SERIAL_NUMBER/8]= {0x55, 0x19, 0x9f, 0xaa}; // marathon2
static byte actual_pad[BITS_PER_SHORT_SERIAL_NUMBER/8]= {0xB5, 0x4B, 0xB4, 0xCE, 0x55}; // marathon infinity

static short long_serial_number_bit_offsets[BITS_PER_LONG_SERIAL_NUMBER]=
{
	34, 74, 52, 15, 67, 4, 43, 48, 33, 78, 64, 40, 60, 26, 7, 70, 19, 44, 20, 30,
	21, 0, 35, 56, 25, 47, 10, 73, 77, 14, 18, 42, 61, 11, 69, 38, 1, 71, 54, 17,
	62, 13, 22, 32, 6, 55, 27, 16, 65, 59, 39, 79, 76, 3, 46, 23, 57, 72, 53, 31,
	51, 5, 75, 12, 28, 68, 36, 41, 63, 49, 29, 66, 8, 2, 37, 50, 58, 9, 45, 24,
};

#ifndef DECODE_ONLY
#define NUMBER_OF_BLACKLISTED_WORDS (sizeof(blacklisted_words)/sizeof(char *))
static char *blacklisted_words[]=
{
	"fuck", "shit", "damn", "anus", "hell", "cunt", "clit", "sex",
	"rape", "pussy", "cock", "doom", "cum", "tit", "twat",
	"crap", "ass", "bitch", "slut", "whore", "piss", "dick", "prick",
	"penis", "fag", "muff"
};
#endif

/* ---------- private prototypes */

static void tokens_to_short_serial_number_and_pad(char *tokens, byte *short_serial_number, byte *inferred_pad);
static void long_serial_number_to_short_serial_number_and_pad(byte *long_serial_number, byte *short_serial_number, byte *inferred_pad);
static void short_serial_number_and_pad_to_tokens(char *tokens, byte *short_serial_number, byte *actual_pad);

static void verify_static_data(void);

static void write_serial_numbers(FILE *stream, short group, long start, long end);

static void generate_long_serial_number_from_short_serial_number(byte *short_serial_number, byte *actual_pad, byte *long_serial_number);
static char extract_token_from_long_serial_number(byte *long_serial_number, short *bit_offset);

static void generate_long_serial_number_from_tokens(char *tokens, byte *long_serial_number);
static void insert_token_into_long_serial_number(char token, byte *long_serial_number, short *bit_offset);
static void extract_short_and_pad_bit_from_long_serial_number(byte *long_serial_number, short *bit_offset, byte *short_serial_number, byte *inferred_pad);

static void valid_serial_number(char *tokens);

static word calculate_complement(long iterations);

static boolean serial_number_contains_blacklisted_words(char *tokens);

/* ---------- code */

#ifndef DECODE_ONLY
void main(
	int argc,
	char **argv)
{
	short error= 0;

	initialize_debugger(TRUE);
	
	if (argc!=5)
	{
		fprintf(stderr, "Usage: %s <group#> <start#> <end#> <destination>\n", argv[0]);
		exit(1);
	}
	else
	{
		long group= atoi(argv[1]);
		long start= atoi(argv[2]), end= atoi(argv[3]);
		FILE *stream= fopen(argv[4], "w");
		
		if (stream)
		{
			if (group>=1 && group<=127)
			{
				if (start>=0 && start<=65535 && end>=0 && end<=65535 && start<end)
				{
					write_serial_numbers(stream, group, start, end);
				}
				else
				{
					fprintf(stderr, "%s: bad [start,end] range [#%d,#%d]", argv[0], start, end);
					error= -1;
				}
			}
			else
			{
				fprintf(stderr, "%s: group must be in (0,127] but got #%d", argv[0], group);
				error= -1;
			}

			fclose(stream);
		}
		else
		{
			fprintf(stderr, "%s: Error opening '%s'", argv[0], argv[4]);
			error= -1;
		}
	}
	
	exit(error);
}

/* ---------- private code */

#ifndef DECODE_ONLY
static void write_serial_numbers(
	FILE *stream,
	short group,
	long start,
	long end)
{
	long count= end-start;

	verify_static_data();
	
	while ((count-= 1)>=0)
	{
		byte short_serial_number[BYTES_PER_SHORT_SERIAL_NUMBER];
		char tokenized_solo_serial_number[TOKENS_PER_SERIAL_NUMBER];
		char tokenized_network_serial_number[TOKENS_PER_SERIAL_NUMBER];
		
		/* build the short solo serial number, tokenize and print it */
		*((short*)short_serial_number)= start;
		short_serial_number[2]= group;
		*((short*)(short_serial_number+3))= calculate_complement(start);
		short_serial_number_and_pad_to_tokens(tokenized_solo_serial_number, short_serial_number, actual_pad);

		/* build the short network serial number, tokenize and print it */
		*((short*)short_serial_number)^= 0xffff;
		short_serial_number[2]= -group;
		*((short*)(short_serial_number+3))^= 0xffff;
		short_serial_number_and_pad_to_tokens(tokenized_network_serial_number, short_serial_number, actual_pad);

		/* verify they are valid */
		valid_serial_number(tokenized_solo_serial_number);
		valid_serial_number(tokenized_network_serial_number);

		if (!serial_number_contains_blacklisted_words(tokenized_solo_serial_number) &&
			!serial_number_contains_blacklisted_words(tokenized_network_serial_number))
		{
			fprintf(stream, "%16.16s\t%16.16s\n", tokenized_solo_serial_number, tokenized_network_serial_number);
		}
		else
		{
			fprintf(stderr, "discarding %16.16s/%16.16s\n", tokenized_solo_serial_number, tokenized_network_serial_number);
		}
		
		start+= 1;
	}

	return;
}

static boolean serial_number_contains_blacklisted_words(
	char *tokens)
{
	char string[TOKENS_PER_SERIAL_NUMBER+1];
	boolean found_blacklisted_word= FALSE;
	short i;
	
	memcpy(string, tokens, TOKENS_PER_SERIAL_NUMBER);
	string[TOKENS_PER_SERIAL_NUMBER]= 0;
	strlwr(string);
	
	for (i= 0; i<NUMBER_OF_BLACKLISTED_WORDS; ++i)
	{
		if (strstr(string, blacklisted_words[i])) found_blacklisted_word= TRUE;
	}
	
	return found_blacklisted_word;
}
#endif

static void valid_serial_number(
	char *tokens)
{
	byte short_serial_number[BYTES_PER_SHORT_SERIAL_NUMBER];
	byte inferred_pad[BYTES_PER_SHORT_SERIAL_NUMBER];
	boolean valid= FALSE;
	
	tokens_to_short_serial_number_and_pad(tokens, short_serial_number, inferred_pad);

	// also allow marathon2 network-only numbers
	if ((!PADS_ARE_EQUAL(actual_pad, inferred_pad ) &&
		!(PADS_ARE_EQUAL(actual_pad_m2, inferred_pad) && ((char) short_serial_number[2])<0)) ||
		!VALID_INVERSE_SEQUENCE(short_serial_number))
	{
		dprintf("serial number %16.16s is invalid, in retrospect", tokens);
	}
	
	return;
}

static void tokens_to_short_serial_number_and_pad(
	char *tokens,
	byte *short_serial_number, // 40 bits, 5 bytes
	byte *inferred_pad) // 40 bits, 5 bytes (== actual_pad for valid serial numbers)
{
	byte long_serial_number[BYTES_PER_LONG_SERIAL_NUMBER];

	generate_long_serial_number_from_tokens(tokens, long_serial_number);
	long_serial_number_to_short_serial_number_and_pad(long_serial_number, short_serial_number, inferred_pad);
	
	return;
}

static void short_serial_number_and_pad_to_tokens(
	char *tokens,
	byte *short_serial_number,
	byte *actual_pad)
{
	byte long_serial_number[BYTES_PER_LONG_SERIAL_NUMBER];
	short bit_offset= 0;
	short token_offset;

	/* generate long form */
	generate_long_serial_number_from_short_serial_number(short_serial_number, actual_pad, &long_serial_number);

	/* break out the tokens, one at a time */	
	for (token_offset= 0; token_offset<TOKENS_PER_SERIAL_NUMBER; ++token_offset)
	{
		tokens[token_offset]= extract_token_from_long_serial_number(long_serial_number, &bit_offset);
	}
	
	return;
}

static void generate_long_serial_number_from_short_serial_number(
	byte *short_serial_number,
	byte *actual_pad,
	byte *long_serial_number)
{
	short bit_offset;
	
	for (bit_offset= 0; bit_offset<BITS_PER_SHORT_SERIAL_NUMBER; ++bit_offset)
	{
		boolean previous_bit= bit_offset ? EXTRACT_BIT(long_serial_number, 2*bit_offset-1) : FALSE;
		boolean data_bit= EXTRACT_BIT(short_serial_number, bit_offset); //^previous_bit;
		
		INSERT_BIT(long_serial_number, 2*bit_offset, data_bit);
		INSERT_BIT(long_serial_number, 2*bit_offset+1, EXTRACT_BIT(actual_pad, bit_offset)^data_bit);
	}
	
//	dprintf("short/pad==>long == %08x%08x%04x", *((long*)long_serial_number), *((long*)(long_serial_number+4)), *((short*)(long_serial_number+8)));

	return;
}

static char extract_token_from_long_serial_number(
	byte *long_serial_number,
	short *bit_offset)
{
	short bit;
	byte raw_token[1];
	
	raw_token[0]= 0;
	for (bit= 0; bit<BITS_PER_TOKEN; ++bit)
	{
		INSERT_BIT(&raw_token, bit, EXTRACT_BIT(long_serial_number,
			long_serial_number_bit_offsets[*bit_offset]));
		*bit_offset+= 1;
	}

	assert(raw_token[0]>=0 && raw_token[0]<NUMBER_OF_TOKENS);
	
	return raw_token_to_ascii_token[raw_token[0]];
}
	
static void verify_static_data(
	void)
{
	short i, j;
	
	/* no duplicate tokens */
	for (i= 0; i<NUMBER_OF_TOKENS; ++i)
	{
		for (j= i+1; j<NUMBER_OF_TOKENS; ++j)
		{
			vassert(raw_token_to_ascii_token[i]!=raw_token_to_ascii_token[j],
				csprintf(temporary, "token[#%d]==token[#%d]", i, j));
		}
	}
	
	/* no duplicate bit assignments */
	for (i= 0; i<BITS_PER_LONG_SERIAL_NUMBER; ++i)
	{
		for (j= i+1; j<BITS_PER_LONG_SERIAL_NUMBER; ++j)
		{
			assert(long_serial_number_bit_offsets[i]!=long_serial_number_bit_offsets[j]);
		}
	}

	/* all bits are assigned */
	for (i= 0; i<BITS_PER_LONG_SERIAL_NUMBER; ++i)
	{
		for (j= 0; j<BITS_PER_LONG_SERIAL_NUMBER; ++j)
		{
			if (long_serial_number_bit_offsets[j]==i) break;
		}
		assert(j!=BITS_PER_LONG_SERIAL_NUMBER);
	}
	
	return;
}

#endif /* DECODE_ONLY */

/* ---------- externally usable code */

static word calculate_complement(
	long iterations)
{
	word seed= 0xfded;
	
	while ((iterations-= 1)>=0) seed= (seed&1) ? ((seed>>1)^0xb400) : (seed>>1);

	return seed;
}

static void long_serial_number_to_short_serial_number_and_pad(
	byte *long_serial_number,
	byte *short_serial_number,
	byte *inferred_pad)
{
	short bit_offset;
	short i;

	// alex paranoia
	for (i= 0;i<BYTES_PER_SHORT_SERIAL_NUMBER;i++)
	{
		short_serial_number[i]= 0;
		inferred_pad[i]= 0;
	}

	for (bit_offset= 0; bit_offset<BITS_PER_SHORT_SERIAL_NUMBER; ++bit_offset)
	{
		INSERT_BIT(short_serial_number, bit_offset, EXTRACT_BIT(long_serial_number, 2*bit_offset));
		INSERT_BIT(inferred_pad, bit_offset, EXTRACT_BIT(long_serial_number, 2*bit_offset+1));
	}
	
	return;
}

static void generate_long_serial_number_from_tokens(
	char *tokens,
	byte *long_serial_number)
{
	short token_offset;
	short bit_offset= 0;

	for (token_offset= 0; token_offset<TOKENS_PER_SERIAL_NUMBER; ++token_offset)
	{
		insert_token_into_long_serial_number(toupper(tokens[token_offset]), long_serial_number, &bit_offset);
	}
	
//	dprintf("tokens==>long == %08x%08x%04x", *((long*)long_serial_number), *((long*)(long_serial_number+4)), *((short*)(long_serial_number+8)));

	for (bit_offset= 0; bit_offset<BITS_PER_LONG_SERIAL_NUMBER; bit_offset+= 2)
	{
		INSERT_BIT(long_serial_number, bit_offset+1, EXTRACT_BIT(long_serial_number, bit_offset)^EXTRACT_BIT(long_serial_number, bit_offset+1));
	}
	
//	dprintf("  (postprocessed into %08x%08x%04x)", *((long*)long_serial_number), *((long*)(long_serial_number+4)), *((short*)(long_serial_number+8)));

	return;
}

static void insert_token_into_long_serial_number(
	char token,
	byte *long_serial_number,
	short *bit_offset)
{
	byte raw_token[1];
	short bit;
	
	for (raw_token[0]= 0;
		raw_token[0]<NUMBER_OF_TOKENS && raw_token_to_ascii_token[raw_token[0]]!=token;
		++raw_token[0]);
	
	for (bit= 0; bit<BITS_PER_TOKEN; ++bit)
	{
		INSERT_BIT(long_serial_number, long_serial_number_bit_offsets[*bit_offset],
			EXTRACT_BIT(&raw_token, bit));
		*bit_offset+= 1;
	}
	
	return;
}
