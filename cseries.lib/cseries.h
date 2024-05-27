#ifndef __CSERIES_H
#define __CSERIES_H

/*
CSERIES.H
Wednesday, December 26, 1990 8:24:58 PM

This is the new global header file for all code which does not rely specifically on
the Macintosh OS.

Tuesday, June 25, 1991 1:53:12 AM
	changes to assert() macro so that we can actually trap errors using assert (ie.
	we differentiate between fatal and non-fatal asserts and provide information as
	to the exact nature of the error).
Thursday, July 18, 1991 11:25:32 AM
	that was a bad idea.  We don’t do that anymore.
Friday, August 9, 1991 11:16:28 PM
	removed random() prototype; this is up to the application (for Minotaur).
Friday, December 20, 1991 3:21:10 PM
	error handling sucks; soon we need to perfect a better scheme.
Monday, May 18, 1992 7:52:19 PM
	added portable memory primitives; pointers and handles.
Friday, August 21, 1992 8:11:56 PM
	removed error codes, this will piss-off minotaur.
Tuesday, September 21, 1993 3:35:23 PM
	MIN, MAX macros added.  more fixed math definitions added.  added rsprintf and getcstr.
Sunday, February 6, 1994 11:07:52 AM
	we don't override malloc and free anymore (use new_pointer and new_handle for portability).
*/

#include <stdio.h>
#include <stdlib.h>

#ifdef powerc
	#define envppc
	#define mpwppc
#else
	#define env68k
	#define mpwc
#endif

/* ---------- constants */

#ifndef TRUE /* For compatibility with PowerPlant's headers, we need the ifndef */
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#define NONE -1

#define KILO 1024
#define MEG (KILO*KILO)
#define GIG (KILO*MEG)

#define MACHINE_TICKS_PER_SECOND 60

/* ---------- macros */

#ifdef DEBUG
	#define halt() _assertion_failure((char *)NULL, __FILE__, __LINE__, TRUE)
	#define vhalt(diag) _assertion_failure(diag, __FILE__, __LINE__, TRUE)
	#define assert(expr) if (!(expr)) _assertion_failure(#expr, __FILE__, __LINE__, TRUE)
	#define vassert(expr,diag) if (!(expr)) _assertion_failure(diag, __FILE__, __LINE__, TRUE)
	#define warn(expr) if (!(expr)) _assertion_failure(#expr, __FILE__, __LINE__, FALSE)
	#define vwarn(expr,diag) if (!(expr)) _assertion_failure(diag, __FILE__, __LINE__, FALSE)
	#define pause() _assertion_failure((char *)NULL, __FILE__, __LINE__, FALSE)
	#define vpause(diag) _assertion_failure(diag, __FILE__, __LINE__, FALSE)
#else
	#define halt()
	#define vhalt(diag)
	#define assert(expr)
	#define vassert(expr,diag)
	#define warn(expr)
	#define vwarn(expr,diag)
	#define pause()
	#define vpause(diag)
#endif

#define SGN(x) ((x)?((x)<0?-1:1):0)

#define SWAP(a,b) a^= b, b^= a, a^= b

#define ABS(x) ((x>=0) ? (x) : -(x))

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define FLOOR(n,floor) ((n)<(floor)?(floor):(n))
#define CEILING(n,ceiling) ((n)>(ceiling)?(ceiling):(n))
#define PIN(n,floor,ceiling) ((n)<(floor) ? (floor) : CEILING(n,ceiling))

#define compact_array(array, element, nmemb, size) if ((element)<(nmemb)-1) \
	memcpy(((byte*)(array))+(element)*(size), ((byte*)(array))+((element)+1)*(size), ((nmemb)-(element)-1)*(size))

#define FLAG(b) (1<<(b))

#define TEST_FLAG16(f, b) ((f)&(word)FLAG(b))
#define SWAP_FLAG16(f, b) ((f)^=(word)FLAG(b))
#define SET_FLAG16(f, b, v) ((v) ? ((f)|=(word)FLAG(b)) : ((f)&=(word)~FLAG(b)))

#define TEST_FLAG32(f, b) ((f)&(unsigned long)FLAG(b))
#define SWAP_FLAG32(f, b) ((f)^=(unsigned long)FLAG(b))
#define SET_FLAG32(f, b, v) ((v) ? ((f)|=(unsigned long)FLAG(b)) : ((f)&=(unsigned long)~FLAG(b)))

/* ---------- fixed math */

#define FIXED_FRACTIONAL_BITS 16
#define FIXED_ONE ((fixed)(1<<FIXED_FRACTIONAL_BITS))
#define FIXED_ONE_HALF ((fixed)(1<<(FIXED_FRACTIONAL_BITS-1)))

#define FIXED_TO_FLOAT(f) (((double)(f))/FIXED_ONE)
#define FLOAT_TO_FIXED(f) ((fixed)((f)*FIXED_ONE))

#define INTEGER_TO_FIXED(s) (((fixed)(s))<<FIXED_FRACTIONAL_BITS)
#define FIXED_INTEGERAL_PART(f) ((short)((f)>>FIXED_FRACTIONAL_BITS))
#define FIXED_TO_INTEGER(f) FIXED_INTEGERAL_PART(f)
#define FIXED_TO_INTEGER_ROUND(f) FIXED_INTEGERAL_PART((f)+FIXED_ONE_HALF)
#define FIXED_FRACTIONAL_PART(f) (((fixed)(f))&(FIXED_ONE-1))

typedef long fixed;

/* ---------- types */

typedef unsigned short word;
typedef unsigned char byte;
typedef byte boolean;

typedef void *handle; // relocatable malloc

/* ---------- limits */

enum
{
	UNSIGNED_LONG_MAX= 4294967295,
	LONG_MAX= 2147483647L,
	LONG_MIN= (-2147483648L),
	LONG_BITS= 32,

	UNSIGNED_SHORT_MAX= 65535,
	SHORT_MAX= 32767,
	SHORT_MIN= (-32768),
	SHORT_BITS= 16,

	UNSIGNED_CHAR_MAX= 255,
	CHAR_MAX= 127,
	CHAR_MIN= (-128),
	CHAR_BITS= 8
};

/* ---------- globals */

extern char temporary[256];

/* ---------- prototypes */

unsigned long machine_tick_count(void);

char *strupr(char *string);
char *strlwr(char *string);

void _assertion_failure(char *assertion, char *file, int line, boolean fatal);

#define malloc(size) new_pointer(size)
#define free(ptr) dispose_pointer(ptr)
void *new_pointer(long size);
void dispose_pointer(void *pointer);

#if 0
handle rmalloc(size);
void rfree(handle h);
void *rlock(handle h);
void runlock(handle h);
#endif

enum // for alert_user
{
	fatalError,
	infoError
};

void alert_user(short type, short resource_number, short error_number, short identifier);

char *getcstr(char *buffer, short collection_number, short string_number);

boolean toggle_debug_status(void);

void initialize_debugger(boolean force_debugger_on);
int dprintf(const char *format, ...);
char *csprintf(char *buffer, char *format, ...);
int rsprintf(char *s, short resource_number, short string_number, ...);

#endif
