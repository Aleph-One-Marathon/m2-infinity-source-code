#ifndef __TEXTURES_H
#define __TEXTURES_H

/*
TEXTURES.H
Saturday, August 20, 1994 12:08:34 PM
*/

/* ---------- pixels */

typedef unsigned char pixel8;
typedef unsigned short pixel16;
typedef unsigned long pixel32;

#define PIXEL8_MAXIMUM_COLORS 256
#define PIXEL16_MAXIMUM_COLORS 32768
#define PIXEL32_MAXIMUM_COLORS 16777216

#define PIXEL16_BITS 5
#define PIXEL32_BITS 8

#define NUMBER_OF_COLOR_COMPONENTS 3
#define PIXEL16_MAXIMUM_COMPONENT 0x1f
#define PIXEL32_MAXIMUM_COMPONENT 0xff

#define RED16(p) ((p)>>10) /* pel must be clean */
#define GREEN16(p) (((p)>>5)&PIXEL16_MAXIMUM_COMPONENT)
#define BLUE16(p) ((p)&PIXEL16_MAXIMUM_COMPONENT)
#define BUILD_PIXEL16(r,g,b) (((r)<<10)|((g)<<5)|(b))
#define RGBCOLOR_TO_PIXEL16(r,g,b) (((pixel16)((r)>>1)&(pixel16)0x7c00)|((pixel16)((g)>>6)&(pixel16)0x03e0)|((pixel16)((b)>>11)&(pixel16)0x1f))

#define RED32(p) ((p)>>16) /* pel must be clean; may be impossible */
#define GREEN32(p) (((p)>>8)&PIXEL32_MAXIMUM_COMPONENT)
#define BLUE32(p) ((p)&PIXEL32_MAXIMUM_COMPONENT)
#define BUILD_PIXEL32(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define RGBCOLOR_TO_PIXEL32(r,g,b) (((((pixel32)(r))<<8)&0x00ff0000) | ((((pixel32)(g)))&0x0000ff00) | ((((pixel32)(b))>>8)&0x000000ff))

/* ---------- color tables */

struct rgb_color
{
	word red, green, blue;
};

struct color_table
{
	short color_count;
	
	struct rgb_color colors[256];
};

/* ---------- structures */

enum /* bitmap flags */
{
	_COLUMN_ORDER_BIT= 0x8000,
	_TRANSPARENT_BIT= 0x4000
};

struct bitmap_definition
{
	short width, height; /* in pixels */
	short bytes_per_row; /* if ==NONE this is a transparent RLE shape */
	
	short flags; /* [column_order.1] [unused.15] */
	short bit_depth; /* should always be ==8 */

	short unused[8];
	
	pixel8 *row_addresses[1];
};

/* ---------- prototypes/TEXTURES.C */

/* assumes pixel data follows bitmap_definition structure immediately */
pixel8 *calculate_bitmap_origin(struct bitmap_definition *bitmap);

/* initialize bytes_per_row, height and row_address[0] before calling */
void precalculate_bitmap_row_addresses(struct bitmap_definition *texture);

void map_bytes(byte *buffer, byte *table, long size);
void remap_bitmap(struct bitmap_definition *bitmap,	pixel8 *table);

void erase_bitmap(struct bitmap_definition *bitmap, long pel);

#endif
