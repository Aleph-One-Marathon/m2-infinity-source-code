/*
LOW_LEVEL_TEXTURES.C
Friday, August 19, 1994 2:05:54 PM

Monday, February 27, 1995 11:40:47 PM  (Jason')
	rob suggests that the PPC might not write-allocate cache lines so we might be faster if we
	read from a location we’re about to write to.  he also suggested a rowbytes of 704 instead
	of 640 off-screen for better cache performance.
*/

/*
we don’t include anything here because we are included in SCOTTISH_TEXTURES.C, with
BIT_DEPTH==8 and BIT_DEPTH==16
*/

#undef PEL
#undef TEXTURE_HORIZONTAL_POLYGON_LINES
#undef TEXTURE_VERTICAL_POLYGON_LINES
#undef TRANSPARENT_TEXTURE_VERTICAL_POLYGON_LINES
#undef RANDOMIZE_VERTICAL_POLYGON_LINES
#undef TINT_VERTICAL_POLYGON_LINES
#undef LANDSCAPE_HORIZONTAL_POLYGON_LINES

#if BIT_DEPTH==32
#define PEL pixel32
#define TEXTURE_HORIZONTAL_POLYGON_LINES _texture_horizontal_polygon_lines32
#define TEXTURE_VERTICAL_POLYGON_LINES _texture_vertical_polygon_lines32
#define TRANSPARENT_TEXTURE_VERTICAL_POLYGON_LINES _transparent_texture_vertical_polygon_lines32
#define RANDOMIZE_VERTICAL_POLYGON_LINES _randomize_vertical_polygon_lines32
#define TINT_VERTICAL_POLYGON_LINES _tint_vertical_polygon_lines32
#define LANDSCAPE_HORIZONTAL_POLYGON_LINES _landscape_horizontal_polygon_lines32
#endif

#if BIT_DEPTH==16
#define PEL pixel16
#define TEXTURE_HORIZONTAL_POLYGON_LINES _texture_horizontal_polygon_lines16
#define TEXTURE_VERTICAL_POLYGON_LINES _texture_vertical_polygon_lines16
#define TRANSPARENT_TEXTURE_VERTICAL_POLYGON_LINES _transparent_texture_vertical_polygon_lines16
#define RANDOMIZE_VERTICAL_POLYGON_LINES _randomize_vertical_polygon_lines16
#define TINT_VERTICAL_POLYGON_LINES _tint_vertical_polygon_lines16
#define LANDSCAPE_HORIZONTAL_POLYGON_LINES _landscape_horizontal_polygon_lines16
#endif

#if BIT_DEPTH==8
#define PEL pixel8
#define TEXTURE_HORIZONTAL_POLYGON_LINES _texture_horizontal_polygon_lines8
#define TEXTURE_VERTICAL_POLYGON_LINES _texture_vertical_polygon_lines8
#define TRANSPARENT_TEXTURE_VERTICAL_POLYGON_LINES _transparent_texture_vertical_polygon_lines8
#define RANDOMIZE_VERTICAL_POLYGON_LINES _randomize_vertical_polygon_lines8
#define TINT_VERTICAL_POLYGON_LINES _tint_vertical_polygon_lines8
#define LANDSCAPE_HORIZONTAL_POLYGON_LINES _landscape_horizontal_polygon_lines8
#endif

#if (BIT_DEPTH==8)
typedef struct {
	unsigned int b0:8;
	unsigned int b1:8;
	unsigned int b2:8;
	unsigned int b3:8;
} vectorized_long;
#endif

#if !defined(EXTERNAL) || BIT_DEPTH==32
void TEXTURE_HORIZONTAL_POLYGON_LINES(
	struct bitmap_definition *texture,
	struct bitmap_definition *screen,
	struct view_data *view,
	struct _horizontal_polygon_line_data *data,
	short y0,
	short *x0_table,
	short *x1_table,
	short line_count)
{
	#pragma unused (view)

	while ((line_count-= 1)>=0)
	{
		short x0= *x0_table++, x1= *x1_table++;
		
		register PEL *shading_table= (PEL *)data->shading_table;
		register PEL *write= (PEL *) screen->row_addresses[y0] + x0;
		register pixel8 *base_address= texture->row_addresses[0];
		register unsigned long source_x= data->source_x;
		register unsigned long source_y= data->source_y;
		register unsigned long source_dx= data->source_dx;
		register unsigned long source_dy= data->source_dy;
		register short count= x1-x0;
		
#if (BIT_DEPTH==8)
#define PIXEL_LOOKUP (((source_y>>(HORIZONTAL_HEIGHT_DOWNSHIFT-7))&(0x7f<<7))+(source_x>>HORIZONTAL_WIDTH_DOWNSHIFT))
		while (((unsigned int) write)&3)
		{
			*write++= shading_table[base_address[PIXEL_LOOKUP]];
			source_x+= source_dx, source_y+= source_dy;
			count--;
		}
		
		if (count>=4)
		{
			register short inner_count= count>>2;
			
			while ((inner_count-= 1)>=0)
			{
				register unsigned long temp;
				
				temp= (shading_table[base_address[PIXEL_LOOKUP]])<<24;
				source_x+= source_dx, source_y+= source_dy;
				temp|= (shading_table[base_address[PIXEL_LOOKUP]]&0xFF)<<16;
				source_x+= source_dx, source_y+= source_dy;
				temp|= (shading_table[base_address[PIXEL_LOOKUP]]&0xFF)<<8;
				source_x+= source_dx, source_y+= source_dy;
				temp|= (shading_table[base_address[PIXEL_LOOKUP]]&0xFF);
				source_x+= source_dx, source_y+= source_dy;
				*((unsigned long *) write)++= temp;
			}
		}
		
		while (count&3)
		{
			*write++= shading_table[base_address[PIXEL_LOOKUP]];
			source_x+= source_dx, source_y+= source_dy;
			count--;
		}
#undef PIXEL_LOOKUP
#else
		while ((count-= 1)>=0)
		{
			*write++= shading_table[base_address[((source_y>>(HORIZONTAL_HEIGHT_DOWNSHIFT-7))&(0x7f<<7))+(source_x>>HORIZONTAL_WIDTH_DOWNSHIFT)]];
//			*write++= shading_table[source_y>>HORIZONTAL_HEIGHT_DOWNSHIFT][source_x>>HORIZONTAL_WIDTH_DOWNSHIFT]];
			source_x+= source_dx, source_y+= source_dy;
		}
#endif		
		data+= 1;
		y0+= 1;
	}
	
	return;
}
#endif

#define LANDSCAPE_WIDTH_BITS 9
#define LANDSCAPE_TEXTURE_WIDTH_DOWNSHIFT (32-LANDSCAPE_WIDTH_BITS)
void LANDSCAPE_HORIZONTAL_POLYGON_LINES(
	struct bitmap_definition *texture,
	struct bitmap_definition *screen,
	struct view_data *view,
	struct _horizontal_polygon_line_data *data,
	short y0,
	short *x0_table,
	short *x1_table,
	short line_count)
{
	register int landscape_texture_width_downshift= texture->height==1024 ? 32-10 : 32-9;

	#pragma unused (view)

	while ((line_count-= 1)>=0)
	{
		short x0= *x0_table++, x1= *x1_table++;
		
		register PEL *shading_table= (PEL *)data->shading_table;
		register PEL *write= (PEL *) screen->row_addresses[y0] + x0;
		register pixel8 *read= texture->row_addresses[data->source_y];
		register unsigned long source_x= data->source_x;
		register unsigned long source_dx= data->source_dx;
		register int count= x1-x0;
		
#if (BIT_DEPTH==8)
#define PIXEL_LOOKUP (source_x>>landscape_texture_width_downshift)
		while ((((unsigned int) write)&3) && ((count-= 1)>=0))
		{
			*write++= shading_table[read[PIXEL_LOOKUP]];
			source_x+= source_dx;
		}
		
		if (count>=4)
		{
			register short inner_count= count>>2;
			
			while ((inner_count-= 1)>=0)
			{
				register unsigned long temp=0;
				
				temp= (shading_table[read[PIXEL_LOOKUP]])<<24;
				source_x+= source_dx;
				temp|= (shading_table[read[PIXEL_LOOKUP]])<<16;
				source_x+= source_dx;
				temp|= (shading_table[read[PIXEL_LOOKUP]])<<8;
				source_x+= source_dx;
				temp|= (shading_table[read[PIXEL_LOOKUP]]);
				source_x+= source_dx;
				*((unsigned long *) write)++= temp;
			}
		}
		
		while ((count&3) && ((count-= 1)>=0))
		{
			*write++= shading_table[read[PIXEL_LOOKUP]];
			source_x+= source_dx;
		}
#undef PIXEL_LOOKUP
#else
		while ((count-= 1)>=0)
		{
			*write++= shading_table[read[source_x>>landscape_texture_width_downshift]];
			source_x+= source_dx;
		}
#endif

		data+= 1;
		y0+= 1;
	}
	
	return;
}

#if !defined(EXTERNAL) || BIT_DEPTH==32
//#define downshift 25
void TEXTURE_VERTICAL_POLYGON_LINES(
	struct bitmap_definition *screen,
	struct view_data *view,
	struct _vertical_polygon_data *data,
	short *y0_table,
	short *y1_table)
{
	struct _vertical_polygon_line_data *line= (struct _vertical_polygon_line_data *) (data+1);
	int bytes_per_row= screen->bytes_per_row;
	int downshift= data->downshift;
	int line_count= data->width;
	boolean aborted= FALSE;
	int x= data->x0;
	int count;
	
	#pragma unused (view)

	while (line_count>0)	
	{
		if (line_count<4 || (x&3) || aborted)
		{
			int y0= *y0_table++, y1= *y1_table++;
			unsigned long texture_y= line->texture_y;
			unsigned long texture_dy= line->texture_dy;
			PEL *write, *shading_table;
			pixel8 *read;

			shading_table= (PEL *)line->shading_table;
			read= line->texture;
			write= (PEL *)screen->row_addresses[y0] + x;

			for (count= y1-y0; count>0; --count)
			{
				*write= shading_table[read[texture_y>>downshift]], (byte *)write+= bytes_per_row;
				texture_y+= texture_dy;
			}
			
			x+= 1;
			line+= 1;
			line_count-= 1;

			aborted= FALSE;
		}
		else
		{
			unsigned long texture_y0= line[0].texture_y, texture_dy0= line[0].texture_dy;
			pixel8 *read0= line[0].texture;
			PEL *shading_table0= (PEL *)line[0].shading_table;
			
			unsigned long texture_y1= line[1].texture_y, texture_dy1= line[1].texture_dy;
			pixel8 *read1= line[1].texture;
			PEL *shading_table1= (PEL *)line[1].shading_table;
			
			unsigned long texture_y2= line[2].texture_y, texture_dy2= line[2].texture_dy;
			pixel8 *read2= line[2].texture;
			PEL *shading_table2= (PEL *)line[2].shading_table;
			
			unsigned long texture_y3= line[3].texture_y, texture_dy3= line[3].texture_dy;
			pixel8 *read3= line[3].texture;
			PEL *shading_table3= (PEL *)line[3].shading_table;
			
			PEL *write;

			int ymax;
			
			/* sync */	
			{
				int y0= y0_table[0], y1= y0_table[1], y2= y0_table[2], y3= y0_table[3];
				PEL *temp_write;
				
				ymax= MAX(y0, y1), ymax= MAX(ymax, y2), ymax= MAX(ymax, y3);
				write= (PEL *)screen->row_addresses[ymax] + x;
				
				{
					int ymin= MIN(y1_table[0], y1_table[1]);
					
					ymin= MIN(ymin, y1_table[2]);
					ymin= MIN(ymin, y1_table[3]);
					
					if (ymin<=ymax)
					{
						aborted= TRUE;
						continue;
					}
				}

				for (count= ymax-y0, temp_write= (PEL *)screen->row_addresses[y0] + x; count>0; --count)
				{
					temp_write[0]= shading_table0[read0[texture_y0>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y0+= texture_dy0;
				}
				
				for (count= ymax-y1, temp_write= (PEL *)screen->row_addresses[y1] + x; count>0; --count)
				{
					temp_write[1]= shading_table1[read1[texture_y1>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y1+= texture_dy1;
				}
				
				for (count= ymax-y2, temp_write= (PEL *)screen->row_addresses[y2] + x; count>0; --count)
				{
					temp_write[2]= shading_table2[read2[texture_y2>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y2+= texture_dy2;
				}
		
				for (count= ymax-y3, temp_write= (PEL *)screen->row_addresses[y3] + x; count>0; --count)
				{
					temp_write[3]= shading_table3[read3[texture_y3>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y3+= texture_dy3;
				}
			}

#if (BIT_DEPTH==8)
			/* parallel map (x4) */
			{
				int dy0= y1_table[0] - ymax;
				int dy1= y1_table[1] - ymax;
				int dy2= y1_table[2] - ymax;
				int dy3= y1_table[3] - ymax;
				
				count= MIN(dy0, dy1), count= MIN(count, dy2), count= MIN(count, dy3);
				ymax+= count;
				
				{
					vectorized_long	temp;
					unsigned long	*long_write= (unsigned long *)write;
					int old_t3= texture_y3>>downshift;
					int old_t2= texture_y2>>downshift;
					int old_t1= texture_y1>>downshift;
					int old_t0= texture_y0>>downshift;

					temp.b0= shading_table3[read3[old_t3]];
					temp.b1= shading_table2[read2[old_t2]];
					temp.b2= shading_table1[read1[old_t1]];
					temp.b3= shading_table0[read0[old_t0]];
					downshift= 25;
					for (; count>0; --count)
					{
						if ((texture_y3>>downshift) != old_t3)
						{
							old_t3= texture_y3>>downshift;
							temp.b0= shading_table3[read3[old_t3]];
						}
						texture_y3+= texture_dy3;

						if ((texture_y2>>downshift) != old_t2)
						{
							old_t2= texture_y2>>downshift;
							temp.b1= shading_table2[read2[old_t2]];
						}
						texture_y2+= texture_dy2;

						if ((texture_y1>>downshift) != old_t1)
						{
							old_t1= texture_y1>>downshift;
							temp.b2= shading_table1[read1[old_t1]];
						}
						texture_y1+= texture_dy1;

						if ((texture_y0>>downshift) != old_t0)
						{
							old_t0= texture_y0>>downshift;
							temp.b3= shading_table0[read0[old_t0]];
						}
						texture_y0+= texture_dy0;
	
						(*(vectorized_long *) write)= temp;
						(byte *)write+= bytes_per_row;
					}
				}
			}
#elif (BIT_DEPTH==16)
			/* parallel map (x4) */
			{
				int dy0= y1_table[0] - ymax;
				int dy1= y1_table[1] - ymax;
				int dy2= y1_table[2] - ymax;
				int dy3= y1_table[3] - ymax;
				unsigned long	temp;
				unsigned long	*long_write= (unsigned long *)write;
				
				count= MIN(dy0, dy1), count= MIN(count, dy2), count= MIN(count, dy3);
				ymax+= count;
				
				for (; count>0; --count)
				{
					temp= shading_table1[read1[texture_y1>>downshift]];
					texture_y1+= texture_dy1;
		
					temp|= (shading_table0[read0[texture_y0>>downshift]])<<16;
					texture_y0+= texture_dy0;

					((unsigned long *) write)[0]= temp;

					temp= shading_table3[read3[texture_y3>>downshift]];
					texture_y3+= texture_dy3;

					temp|= (shading_table2[read2[texture_y2>>downshift]])<<16;
					texture_y2+= texture_dy2;

					((unsigned long *) write)[1]= temp;
					(byte *)write+= bytes_per_row;
				}
			}
#else
			/* parallel map (x4) */
			{
				int dy0= y1_table[0] - ymax;
				int dy1= y1_table[1] - ymax;
				int dy2= y1_table[2] - ymax;
				int dy3= y1_table[3] - ymax;
				
				count= MIN(dy0, dy1), count= MIN(count, dy2), count= MIN(count, dy3);
				ymax+= count;
				
				for (; count>0; --count)
				{
					write[0]= shading_table0[read0[texture_y0>>downshift]];
					texture_y0+= texture_dy0;
		
					write[1]= shading_table1[read1[texture_y1>>downshift]];
					texture_y1+= texture_dy1;
		
					write[2]= shading_table2[read2[texture_y2>>downshift]];
					texture_y2+= texture_dy2;
		
					write[3]= shading_table3[read3[texture_y3>>downshift]];
					texture_y3+= texture_dy3;
					
					(byte *)write+= bytes_per_row;
				}
			}
#endif

			/* desync */	
			{
				PEL *temp_write;
				
				for (count= y1_table[0] - ymax, temp_write= write; count>0; --count)
				{
					temp_write[0]= shading_table0[read0[texture_y0>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y0+= texture_dy0;
				}
				
				for (count= y1_table[1] - ymax, temp_write= write; count>0; --count)
				{
					temp_write[1]= shading_table1[read1[texture_y1>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y1+= texture_dy1;
				}
				
				for (count= y1_table[2] - ymax, temp_write= write; count>0; --count)
				{
					temp_write[2]= shading_table2[read2[texture_y2>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y2+= texture_dy2;
				}
		
				for (count= y1_table[3] - ymax, temp_write= write; count>0; --count)
				{
					temp_write[3]= shading_table3[read3[texture_y3>>downshift]], (byte *)temp_write+= bytes_per_row;
					texture_y3+= texture_dy3;
				}
			}

			y0_table+= 4, y1_table+= 4;
			line_count-= 4;
			line+= 4;
			x+= 4;
		}
	}
	
	return;
}
//#undef downshift
#endif

#if !defined(EXTERNAL) || BIT_DEPTH==32
void TRANSPARENT_TEXTURE_VERTICAL_POLYGON_LINES(
	struct bitmap_definition *screen,
	struct view_data *view,
	struct _vertical_polygon_data *data,
	short *y0_table,
	short *y1_table)
{
	struct _vertical_polygon_line_data *line= (struct _vertical_polygon_line_data *) (data+1);
	int bytes_per_row= screen->bytes_per_row;
	int downshift= data->downshift;
	int line_count= data->width;
	boolean aborted= FALSE;
	int x= data->x0;
	pixel8 pixel;
	int count;
	
	#pragma unused (view)

	while (line_count>0)	
	{
		if (line_count<4 || (x&3) || aborted)
		{
			int y0= *y0_table++, y1= *y1_table++;
			unsigned long texture_y= line->texture_y;
			unsigned long texture_dy= line->texture_dy;
			PEL *write, *shading_table;
			pixel8 *read;

			shading_table= (PEL *)line->shading_table;
			read= line->texture;
			write= (PEL *)screen->row_addresses[y0] + x;

			for (count= y1-y0; count>0; --count)
			{
				if (pixel= read[texture_y>>downshift]) *write= shading_table[pixel];
				(byte *)write+= bytes_per_row;
				texture_y+= texture_dy;
			}
			
			x+= 1;
			line+= 1;
			line_count-= 1;
			
			aborted= FALSE;
		}
		else
		{
			unsigned long texture_y0= line[0].texture_y, texture_dy0= line[0].texture_dy;
			pixel8 *read0= line[0].texture;
			PEL *shading_table0= (PEL *)line[0].shading_table;
			
			unsigned long texture_y1= line[1].texture_y, texture_dy1= line[1].texture_dy;
			pixel8 *read1= line[1].texture;
			PEL *shading_table1= (PEL *)line[1].shading_table;
			
			unsigned long texture_y2= line[2].texture_y, texture_dy2= line[2].texture_dy;
			pixel8 *read2= line[2].texture;
			PEL *shading_table2= (PEL *)line[2].shading_table;
			
			unsigned long texture_y3= line[3].texture_y, texture_dy3= line[3].texture_dy;
			pixel8 *read3= line[3].texture;
			PEL *shading_table3= (PEL *)line[3].shading_table;
			
			PEL *write;

			int ymax;
			
			/* sync */	
			{
				int y0= y0_table[0], y1= y0_table[1], y2= y0_table[2], y3= y0_table[3];
				PEL *temp_write;
				
				ymax= MAX(y0, y1), ymax= MAX(ymax, y2), ymax= MAX(ymax, y3);
				write= (PEL *)screen->row_addresses[ymax] + x;

				{
					int ymin= MIN(y1_table[0], y1_table[1]);
					
					ymin= MIN(ymin, y1_table[2]);
					ymin= MIN(ymin, y1_table[3]);
					
					if (ymin<=ymax)
					{
						aborted= TRUE;
						continue;
					}
				}
				
				for (count= ymax-y0, temp_write= (PEL *)screen->row_addresses[y0] + x; count>0; --count)
				{
					if (pixel= read0[texture_y0>>downshift]) temp_write[0]= shading_table0[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y0+= texture_dy0;
				}
				
				for (count= ymax-y1, temp_write= (PEL *)screen->row_addresses[y1] + x; count>0; --count)
				{
					if (pixel= read1[texture_y1>>downshift]) temp_write[1]= shading_table1[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y1+= texture_dy1;
				}
				
				for (count= ymax-y2, temp_write= (PEL *)screen->row_addresses[y2] + x; count>0; --count)
				{
					if (pixel= read2[texture_y2>>downshift]) temp_write[2]= shading_table2[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y2+= texture_dy2;
				}
		
				for (count= ymax-y3, temp_write= (PEL *)screen->row_addresses[y3] + x; count>0; --count)
				{
					if (pixel= read3[texture_y3>>downshift]) temp_write[3]= shading_table3[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y3+= texture_dy3;
				}
			}

			/* parallel map (x4) */
			{
				int dy0= y1_table[0] - ymax;
				int dy1= y1_table[1] - ymax;
				int dy2= y1_table[2] - ymax;
				int dy3= y1_table[3] - ymax;
				
				count= MIN(dy0, dy1), count= MIN(count, dy2), count= MIN(count, dy3);
				ymax+= count;
				
				for (; count>0; --count)
				{
					if (pixel= read0[texture_y0>>downshift]) write[0]= shading_table0[pixel];
					texture_y0+= texture_dy0;
		
					if (pixel= read1[texture_y1>>downshift]) write[1]= shading_table1[pixel];
					texture_y1+= texture_dy1;
		
					if (pixel= read2[texture_y2>>downshift]) write[2]= shading_table2[pixel];
					texture_y2+= texture_dy2;
		
					if (pixel= read3[texture_y3>>downshift]) write[3]= shading_table3[pixel];
					texture_y3+= texture_dy3;
					
					(byte *)write+= bytes_per_row;
				}
			}

			/* desync */	
			{
				PEL *temp_write;
				
				for (count= y1_table[0] - ymax, temp_write= write; count>0; --count)
				{
					if (pixel= read0[texture_y0>>downshift]) temp_write[0]= shading_table0[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y0+= texture_dy0;
				}
				
				for (count= y1_table[1] - ymax, temp_write= write; count>0; --count)
				{
					if (pixel= read1[texture_y1>>downshift]) temp_write[1]= shading_table1[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y1+= texture_dy1;
				}
				
				for (count= y1_table[2] - ymax, temp_write= write; count>0; --count)
				{
					if (pixel= read2[texture_y2>>downshift]) temp_write[2]= shading_table2[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y2+= texture_dy2;
				}
		
				for (count= y1_table[3] - ymax, temp_write= write; count>0; --count)
				{
					if (pixel= read3[texture_y3>>downshift]) temp_write[3]= shading_table3[pixel];
					(byte *)temp_write+= bytes_per_row;
					texture_y3+= texture_dy3;
				}
			}

			y0_table+= 4, y1_table+= 4;
			line_count-= 4;
			line+= 4;
			x+= 4;
		}
	}
	
	return;
}
#endif

void TINT_VERTICAL_POLYGON_LINES(
	struct bitmap_definition *screen,
	struct view_data *view,
	struct _vertical_polygon_data *data,
	short *y0_table,
	short *y1_table,
	word transfer_data)
{
	short tint_table_index= transfer_data&0xff;
	struct _vertical_polygon_line_data *line= (struct _vertical_polygon_line_data *) (data+1);
	register short bytes_per_row= screen->bytes_per_row;
	register int downshift= data->downshift;
	int line_count= data->width;
	int x= data->x0;

#if BIT_DEPTH==8
	register PEL *tint_tables= (PEL *)line->shading_table + tint_table_index*sizeof(struct tint_table8);
#endif

#if BIT_DEPTH==16
	register struct tint_table16 *tint_tables= (struct tint_table16 *)line->shading_table + (tint_table_index<<1);
#endif

#if BIT_DEPTH==32
	register struct tint_table32 *tint_tables= (struct tint_table32 *)line->shading_table + (tint_table_index<<3);
#endif

	assert(tint_table_index>=0 && tint_table_index<number_of_shading_tables);

	while ((line_count-= 1)>=0)
	{
		short y0= *y0_table++, y1= *y1_table++;
		register PEL *write= (PEL *) screen->row_addresses[y0] + x;
		register pixel8 *read= line->texture;
		register fixed texture_y= line->texture_y, texture_dy= line->texture_dy;
		register short count= y1-y0;

		while ((count-=1)>=0)
		{
			if (read[FIXED_INTEGERAL_PART(texture_y)])
			{
#if BIT_DEPTH==8			
				*write= tint_tables[*write];
#endif

#if BIT_DEPTH==16
				register PEL pixel= *write;
				*write= tint_tables->red[RED16(pixel)] | tint_tables->green[GREEN16(pixel)] |
					tint_tables->blue[BLUE16(pixel)];
#endif

#if BIT_DEPTH==32
				register PEL pixel= *write;
				*write= tint_tables->red[RED32(pixel)] | tint_tables->green[GREEN32(pixel)] |
					tint_tables->blue[BLUE32(pixel)];
#endif
			}

			(byte *)write+= bytes_per_row;
			texture_y+= texture_dy;
		}

		line+= 1;
		x+= 1;
	}
	
	return;
}

void RANDOMIZE_VERTICAL_POLYGON_LINES(
	struct bitmap_definition *screen,
	struct view_data *view,
	struct _vertical_polygon_data *data,
	short *y0_table,
	short *y1_table,
	word transfer_data)
{
	struct _vertical_polygon_line_data *line= (struct _vertical_polygon_line_data *) (data+1);
	register short bytes_per_row= screen->bytes_per_row;
	register int downshift= data->downshift;
	int line_count= data->width;
	int x= data->x0;
	register word seed= texture_random_seed;
	register word drop_less_than= transfer_data;

	while ((line_count-= 1)>=0)
	{
		short y0= *y0_table++, y1= *y1_table++;
		register PEL *write= (PEL *) screen->row_addresses[y0] + x;
		register pixel8 *read= line->texture;
		register fixed texture_y= line->texture_y, texture_dy= line->texture_dy;
		register short count= y1-y0;

		while ((count-=1)>=0)
		{
			if (read[FIXED_INTEGERAL_PART(texture_y)])
			{
#if BIT_DEPTH==32
				if (seed>=drop_less_than) *write= (pixel32)seed^(((pixel32)seed)<<8);
#else
				if (seed>=drop_less_than) *write= seed;
#endif
				if (seed&1) seed= (seed>>1)^0xb400; else seed= seed>>1;
			}

			(byte *)write+= bytes_per_row;
			texture_y+= texture_dy;
		}

		line+= 1;
		x+= 1;
	}
	
	texture_random_seed= seed;

	return;
}
