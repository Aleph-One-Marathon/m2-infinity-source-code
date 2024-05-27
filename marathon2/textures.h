/*
TEXTURES.H
Saturday, August 20, 1994 12:08:34 PM
*/

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