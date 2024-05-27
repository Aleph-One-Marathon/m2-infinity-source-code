/*
BYTE_SWAPPING.H
Tuesday, August 22, 1995 9:20:34 AM  (Jason)
*/

/* ---------- constants */

enum // _bs_field constants
{
	// positive numbers are runs (of bytes) to leave untouched

	_byte= 1,
	_2byte= -2,
	_4byte= -4
};

/* ---------- macros */

#define SWAP2(q) (((q)>>8) | (((q)<<8)&0xff00))
#define SWAP4(q) (((q)>>24) | (((q)>>8)&0xff00) | (((q)<<8)&0x00ff00) | (((q)<<24)&0xff000000))

/* ---------- types */

typedef short _bs_field;

/* ---------- prototypes/BYTE_SWAPPING.H */

#ifdef mac
#define byte_swap_data(data, size, nmemb, fields)
#define byte_swap_memory(data, type, nmemb)
#endif

#ifdef win
void byte_swap_data(void *data, long size, long nmemb, _bs_field *fields);
void byte_swap_memory(void *data, _bs_field type, long nmemb);
#endif
