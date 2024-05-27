/*
BYTE_SWAPPING.C
Tuesday, August 22, 1995 8:58:12 AM  (Jason)
*/

#include "cseries.h"

#include "byte_swapping.h"

/* ---------- code */

void byte_swap_memory(
	void *data,
	_bs_field type,
	long nmemb)
{
	switch (type)
	{
		case _2byte:
		{
			word *ptr= data;
			
			for (; nmemb>0; --nmemb)
			{
				word q= *ptr;
				*ptr++= SWAP2(q);
			}
			
			break;
		}
		case _4byte:
		{
			unsigned long *ptr= data;
			
			for (; nmemb>0; --nmemb)
			{
				unsigned long q= *ptr;
				*ptr++= SWAP4(q);
			}
			
			break;
		}
		
		default:
			halt();
	}
	
	return;
}

void byte_swap_data(
	byte *data,
	long size,
	long nmemb,
	_bs_field *fields)
{
	long i;
	
	for (i= 0; i<nmemb; ++i)
	{
		_bs_field *current_field= fields;
		int count;
		
		for (count= 0; count<size; )
		{
			_bs_field field= *fields++;
			
			if (field<0)
			{
				switch (field)
				{
					case _2byte:
					{
						word q= *((word *)data);
						*((word *)data)= SWAP2(q);
						data+= sizeof(word), count+= sizeof(word);
						
						break;
					}
					
					case _4byte:
					{
						unsigned long q= *((unsigned long *)data);
						*((unsigned long *)data)= SWAP4(q);
						data+= sizeof(long), count+= sizeof(long);
						
						break;
					}
					
					default:
						halt();
				}
			}
			else
			{
				count+= field, data+= field;
			}
		}
		
		assert(count==size);
	}
	
	return;
}
