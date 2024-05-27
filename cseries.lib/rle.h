#ifndef __RLE_H
#define __RLE_H

/*
RLE.H
Sunday, December 15, 1991 12:10:51 AM
*/

long get_destination_size(byte *compressed);

long compress_bytes(byte *raw, long raw_size, byte *compressed, long maximum_compressed_size);
void uncompress_bytes(byte *compressed, byte *raw);

#endif
