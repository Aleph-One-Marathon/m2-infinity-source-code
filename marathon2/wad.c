/*
	WAD.C
	Thursday, June 30, 1994 10:54:39 PM

	Tuesday, December 13, 1994 4:31:46 PM- allowed for application specific data in the directory
		data.  This lets me put the names and entry flags in one tight logical place.

	Sunday, February 5, 1995 1:55:01 AM- allow for offset for inplace creation of data, added
		version control so that I can read things that are old, implemented a checksum value 
		for the header, allowed for poor man's search criteria.
		
		Three open calls:
			1) open_wad_file_for_reading -> standard open call.
			2) open_union_wad_file_for_reading-> opens multiple files, based on file type and 
				alphabetical.
			3) open_union_wad_file_for_reading_by_list-> opens multiple files, based on array 
				passed in.

Future Options:
• Reentrancy
• Use malloc to actually make it possible to read the data as changes are made in the future?

	Saturday, October 28, 1995 1:13:38 PM- whoops.  Had a huge memory leak in the inflate wad
		data.  I’m fired.
	
*/

// Note that level_transition_malloc is specific to marathon...

#include "macintosh_cseries.h"

#include <string.h>

#include "wad.h"
#include "tags.h"
#include "crc.h"
#include "game_errors.h"
#include "interface.h" // for strERRORS

#ifdef mpwc
#pragma segment file_io
#endif

/* ---------------- private structures */
struct wad_internal_data {
	short file_count;
	FileDesc files[MAXIMUM_UNION_WADFILES];
};

/* ---------------- private global data */
struct wad_internal_data *internal_data[MAXIMUM_OPEN_WADFILES]= {NULL, NULL, NULL};

/* ---------------- private prototypes */
static long calculate_directory_offset(struct wad_header *header, short index);
static void dump_raw_wad(byte *wad);
static short get_directory_base_length(struct wad_header *header);
static short get_entry_header_length(struct wad_header *header);
static FileError read_indexed_directory_data(fileref ref, struct wad_header *header,
	short index, struct directory_entry *entry);
static struct wad_internal_data *get_internal_data_for_file_id(short file_id);
static struct wad_internal_data *allocate_internal_data_for_file_id(short file_id);
void free_internal_data_for_file_id(short file_id);
static fileref find_available_union_refnum(void);
static void dump_union_ref(fileref union_ref);
static long calculate_raw_wad_length(struct wad_header *file_header, byte *wad);
static FileError read_indexed_wad_from_file_into_buffer(fileref file_id, 
	struct wad_header *header, short index, void *buffer, long *length);
static short count_raw_tags(byte *raw_wad);
static struct wad_data *convert_wad_from_raw(struct wad_header *header, byte *data,	long wad_start_offset,
	long raw_length);
static struct wad_data *convert_wad_from_raw_modifiable(struct wad_header *header, byte *raw_wad, long raw_length);
static void patch_wad_from_raw(struct wad_header *header, byte *raw_wad, struct wad_data *read_wad);
static FileError size_of_indexed_wad(fileref file_id, struct wad_header *header, short index, 
	long *length);

static FileError write_to_file(fileref file_id, long offset, void *data, long length);
static FileError read_from_file(fileref file_id, long offset, void *data, long length);
	
/* ------------------ Code Begins */
boolean read_wad_header(
	fileref file_id, 
	struct wad_header *header)
{
	boolean union_file= FALSE;
	FileError error;
	boolean success= TRUE;

	if(file_id<0) 
	{
		/* This is a union refnum.. */
		struct wad_internal_data *data= get_internal_data_for_file_id(file_id);

		assert(data);
		file_id= open_wad_file_for_reading(&data->files[0]);
		union_file= TRUE;
	}
	assert(file_id>=0);
	error= read_from_file(file_id, 0, header, sizeof(struct wad_header));
	
	if(union_file)
	{
		close_wad_file(file_id);
	}
	
	if(error)
	{
		set_game_error(systemError, error);
		success= FALSE;
	} else {
		if(header->version>CURRENT_WADFILE_VERSION && header->version!=INFINITY_WADFILE_VERSION)
		{
			set_game_error(gameError, errUnknownWadVersion);
			success= FALSE;
		}
	}
	
	return success;
}

extern void *level_transition_malloc(size_t size);

/* This could be improved.  Under the current implementation, it requires 2X sizeof level worth */
/*  of memory to load... (This makes writing wads easier, but isn't really useful for loading */
/* Note that this does the correct thing for union wadfiles... */
struct wad_data *read_indexed_wad_from_file(
	fileref file_id, 
	struct wad_header *header, 
	short index,
	boolean read_only)
{
	struct wad_data *read_wad= (struct wad_data *) NULL;
	byte *raw_wad;
	long length;
	FileError error= 0;

	if(file_id>=0) /* NOT a union wadfile... */
	{
		error= size_of_indexed_wad(file_id, header, index, &length);
		if(!error)
		{
#if 0
			raw_wad= (byte *) malloc(length);
#else
			raw_wad= (byte *) level_transition_malloc(length);
#endif
			if(raw_wad)
			{
				/* Read into the buffer */
				error= read_indexed_wad_from_file_into_buffer(file_id, header, index, raw_wad, &length);
				if(!error)
				{
					/* Got the raw wad. Convert it into our internal representation... */
					if(read_only)
					{
						read_wad= convert_wad_from_raw(header, raw_wad, 0, length);
					} else {
						read_wad= convert_wad_from_raw_modifiable(header, raw_wad, length);

						/* Free it! */
						free(raw_wad);
					}

					if(!read_wad)
					{
						/* Error.. */
						error= memory_error();
					}
				}
			} else {
				error= memory_error();
			}
		}
	} else {
		/* This is a union wadfile! Danger! */
		struct wad_internal_data *union_wad_data;
		short base_ref;
	
		/* Get the data.. */
		union_wad_data= get_internal_data_for_file_id(file_id);
		assert(union_wad_data);
		assert(union_wad_data->file_count);
		
		/* Call myself, to load in the base file... */
		base_ref= open_wad_file_for_reading(&union_wad_data->files[0]);
		if(base_ref>=0)
		{
			read_wad= read_indexed_wad_from_file(base_ref, header, index, read_only);
			close_wad_file(base_ref);
		}
		
		/* We have now read in the base wad.... */
		if(read_wad)
		{
			short index;
			
			/* Loop through, patching in place... */
			/* Note that no errors can happen on patch loads... */
			for(index= 1; index<union_wad_data->file_count; ++index)
			{
				short file_ref;

				file_ref= open_wad_file_for_reading(&union_wad_data->files[index]);
				if(file_ref>=0) 
				{
					error= size_of_indexed_wad(file_ref, header, index, &length);
					if(!error)
					{
						raw_wad= (byte *) malloc(length);
						if(raw_wad)
						{	
							/* Read into the buffer */
							error= read_indexed_wad_from_file_into_buffer(file_ref, header, index, raw_wad, &length);
							if(!error)
							{
								/* Success! */
								/* Got the raw wad. Patch it into our internal representation... */
								patch_wad_from_raw(header, raw_wad, read_wad);
							}
							
							free(raw_wad);
						}
					}
					
					close_wad_file(file_ref);
				} else {
					dprintf("Unable to open patch file: %P!", union_wad_data->files[index].name);
				}
			}
		}
	}

	if(error)
	{
		set_game_error(systemError, error);
	}
	
	return read_wad;
}

void *extract_type_from_wad(
	struct wad_data *wad,
	WadDataType type, 
	long *length)
{
	void *return_value= NULL;
	short index;
	
	*length= 0l;
	
	assert(wad);
	for(index= 0; index<wad->tag_count; ++index)
	{
		if(wad->tag_data[index].tag==type)
		{
			return_value= wad->tag_data[index].data;
			*length= wad->tag_data[index].length;
			break;
		}
	}
	
	return return_value;
}

boolean wad_file_has_checksum(
	FileDesc *file, 
	unsigned long checksum)
{
	boolean has_checksum= FALSE;

	if(checksum==read_wad_file_checksum(file))
	{
		has_checksum= TRUE;
	}
	
	return has_checksum;
}

unsigned long read_wad_file_checksum(
	FileDesc *file)
{
	fileref file_id;
	struct wad_header header;
	unsigned long checksum= 0l;

	file_id= open_wad_file_for_reading(file);
	if(file_id>=0)
	{
		if(read_wad_header(file_id, &header))
		{
			checksum= header.checksum;
		}
		
		close_wad_file(file_id);
	}
	
	return checksum;
}

unsigned long read_wad_file_parent_checksum(
	FileDesc *file)
{
	fileref file_id;
	struct wad_header header;
	unsigned long checksum= 0l;

	file_id= open_wad_file_for_reading(file);
	if(file_id>=0)
	{
		if(read_wad_header(file_id, &header))
		{
			checksum= header.parent_checksum;
		}
		
		close_wad_file(file_id);
	}
	
	return checksum;
}

boolean wad_file_has_parent_checksum(
	FileDesc *file, 
	unsigned long parent_checksum)
{
	fileref file_id;
	boolean has_checksum= FALSE;
	struct wad_header header;

	file_id= open_wad_file_for_reading(file);
	if(file_id>=0)
	{
		if(read_wad_header(file_id, &header))
		{
			if(header.parent_checksum==parent_checksum)
			{
				/* Found a match!  */
				has_checksum= TRUE;
			}
		}
		
		close_wad_file(file_id);
	}
	
	return has_checksum;
}

/* ------------------- Union Wadfile Functions! ---------------- */
/* 0 is returned on an error. */
fileref open_union_wad_file_for_reading(
	FileDesc *base_file)
{
	fileref file_id= 0;
	fileref parent_file_id;
	
	/* Initially just open the base file, to make sure that it exists */
	parent_file_id= open_wad_file_for_reading(base_file);
	if(parent_file_id>=0) 
	{
		struct wad_header header;
		
		if(read_wad_header(parent_file_id, &header))
		{
			long checksum= header.checksum;
		
			/* Get the next available union refnum */
			file_id= find_available_union_refnum();
		
			/* If we didn't get too many.. */
			if(file_id<0)
			{
				struct wad_internal_data *data;
				FileError error;
			
				/* Allocate our internal data */
				data= get_internal_data_for_file_id(file_id);
				if(!data) data= allocate_internal_data_for_file_id(file_id);
				assert(data);
			
				/* Setup the base file.. */
				data->file_count= 1;
				data->files[0]= *base_file;
				
				/* Enumerate the rest of the files */
				error= find_other_entries_that_reference_checksum(checksum, data->files, &data->file_count);

				if(error)
				{
					set_game_error(systemError, error);
				}
			} else {
				file_id= 0;
				set_game_error(gameError, errTooManyOpenFiles);
			}
		}

		close_wad_file(parent_file_id);
	}

if(file_id<0) dump_union_ref(file_id);
	
	return file_id;
}

fileref open_union_wad_file_for_reading_by_list(
	FileDesc *files,
	short count)
{
	fileref file_id= 0;
	fileref parent_file_id;
	
	assert(count>=1);
	
	/* Initially just open the base file, to make sure that it exists */
	parent_file_id= open_wad_file_for_reading(&files[0]);
	if(parent_file_id>=0) 
	{
		struct wad_header header;
		
		if(read_wad_header(parent_file_id, &header))
		{
			long checksum= header.checksum;
		
			/* Get the next available union refnum */
			file_id= find_available_union_refnum();
		
			/* If we didn't get too many.. */
			if(file_id<0)
			{
				struct wad_internal_data *data;
				short actual_count, index;
			
				/* Allocate our internal data */
				data= get_internal_data_for_file_id(file_id);
				if(!data) data= allocate_internal_data_for_file_id(file_id);
				assert(data);
			
				/* Setup the base file.. */
				data->file_count= 1;
				data->files[0]= files[0];
				
				/* Setup the rest of them.. */
				actual_count= 1;
				for(index= 1; index<count; ++index)
				{
					/* Only add it if the checksum is valid.. */
					if(wad_file_has_parent_checksum(&files[index], checksum))
					{
						data->files[actual_count++]= files[index];
					}
				}
				data->file_count= actual_count;
			} else {
				file_id= 0;
				set_game_error(gameError, errTooManyOpenFiles);
			}
		}

		close_wad_file(parent_file_id);
	}

if(file_id<0) dump_union_ref(file_id);
	
	return file_id;
}

/* ------------ Writing functions */
struct wad_data *create_empty_wad(void)
{
	struct wad_data *wad;

	wad= (struct wad_data *) malloc(sizeof(struct wad_data));
	if(wad)
	{
		memset(wad, 0, sizeof(struct wad_data)); /* IMPORTANT! */
	}

	return wad;
} 

void fill_default_wad_header(
	FileDesc *file, 
	short wadfile_version,
	short data_version, 
	short wad_count,
	short application_directory_data_size,
	struct wad_header *header)
{
	memset(header, 0, sizeof(struct wad_header));
	header->version= wadfile_version;
	header->data_version= data_version;
	memcpy(header->file_name, file->name, file->name[0]+1);
	p2cstr((StringPtr)header->file_name);
	header->wad_count= wad_count;
	header->application_specific_directory_data_size= application_directory_data_size;					

	header->entry_header_size= get_entry_header_length(header);
	if(!header->entry_header_size) 
	{
		/* Default.. */
		header->entry_header_size= sizeof(struct entry_header);
	}

	header->directory_entry_base_size= get_directory_base_length(header);
	if(!header->directory_entry_base_size)
	{
		header->directory_entry_base_size= sizeof(struct directory_entry);
	}

	/* Things left for caller to fill in: */
	/* unsigned long checksum, long directory_offset, unsigned long parent_checksum */
};

boolean write_wad_header(
	fileref file_id, 
	struct wad_header *header)
{
	FileError error;
	boolean success= TRUE;
	
	error= write_to_file(file_id, 0, header, sizeof(struct wad_header));
	if(error)
	{
		set_game_error(systemError, error);
		success= FALSE;
	}
	
	return success;
}

boolean write_directorys(
	fileref file_id, 
	struct wad_header *header,
	void *entries)
{
	long size_to_write= get_size_of_directory_data(header);
	FileError error;
	boolean success= TRUE;
	
	assert(header->version>=WADFILE_HAS_DIRECTORY_ENTRY);
	error= write_to_file(file_id, header->directory_offset, entries, 
		size_to_write);
	if(error)
	{
		set_game_error(systemError, error);
		success= FALSE;
	}
	
	return success;
}

/* Note wad_count better be correct! */
long get_size_of_directory_data(
	struct wad_header *header)
{
	short base_entry_size= get_directory_base_length(header);

	assert(header->wad_count);
	assert(header->version>=WADFILE_HAS_DIRECTORY_ENTRY || header->application_specific_directory_data_size==0);

	return (header->wad_count*
		(header->application_specific_directory_data_size+base_entry_size));
}

void *get_indexed_directory_data(
	struct wad_header *header,
	short index,
	void *directories)
{
	byte *data_ptr= (byte *)directories;
	short base_entry_size= get_directory_base_length(header);

	assert(header->version>=WADFILE_HAS_DIRECTORY_ENTRY);
	assert(index>=0 && index<header->wad_count);
	data_ptr += index*(header->application_specific_directory_data_size+base_entry_size);
	data_ptr += base_entry_size; /* Because the application specific junk follows the standard entries */

	return ((void *) data_ptr);
}

void set_indexed_directory_offset_and_length(
	struct wad_header *header,
	void *entries,
	short index,
	long offset,
	long length,
	short wad_index)
{
	byte *data_ptr= (byte *)entries;
	struct directory_entry *entry;
	long data_offset;

	assert(header->version>=WADFILE_HAS_DIRECTORY_ENTRY);
	
	/* calculate_directory_offset is for the file, by subtracting the base, we get the actual offset.. */
	data_offset= calculate_directory_offset(header, index) - header->directory_offset;

	data_ptr+= data_offset;
	entry= (struct directory_entry *) data_ptr;
	
	entry->length= length;
	entry->offset_to_start= offset;
	
	if(header->version>=WADFILE_SUPPORTS_OVERLAYS)
	{
		entry->index= wad_index;
	}
	
	return;
}

void *read_directory_data(
	short file_ref,
	struct wad_header *header)
{
	long size;
	byte *data;
	
	assert(header->version>=WADFILE_HAS_DIRECTORY_ENTRY);
	
	size= get_size_of_directory_data(header);
	data= (byte *)malloc(size);
	if(data)
	{
		FileError error;
		
		error= read_from_file(file_ref, header->directory_offset, data, size);
		assert(!error);
	}

	return data;
}

struct wad_data *append_data_to_wad(
	struct wad_data *wad, 
	WadDataType type, 
	void *data, 
	long size,
	long offset) /* Allows for inplace creation of wadfiles */
{
	short index;

	assert(size); /* You can't append zero length data anymore! */
	assert(wad);
	assert(!wad->read_only_data);

	/* Find the index to replace */
	for(index= 0; index<wad->tag_count; ++index)
	{
		if(wad->tag_data[index].tag==type) 
		{
			/* Just free it, and let it go. */
			free(wad->tag_data[index].data);
			break;
		}
	}
	
	/* If we are appending... */
	if(index==wad->tag_count)
	{
		struct tag_data *old_data= wad->tag_data;

		wad->tag_count++;
		wad->tag_data= (struct tag_data *) malloc(wad->tag_count*sizeof(struct tag_data));

		if(!wad->tag_data)
		{
			alert_user(fatalError, strERRORS, outOfMemory, memory_error());
		}

		assert(wad->tag_data);
		memset(wad->tag_data, 0, wad->tag_count*sizeof(struct tag_data));
		if(old_data)
		{
			memcpy(wad->tag_data, old_data, (wad->tag_count-1)*sizeof(struct tag_data));
			free(old_data);
		}
	}

	/* Copy it in.. */
	assert(index>=0 && index<wad->tag_count);
	wad->tag_data[index].data= (byte *) malloc(size);
	if(!wad->tag_data[index].data)
	{
		alert_user(fatalError, strERRORS, outOfMemory, memory_error());
	}
	assert(wad->tag_data[index].data);
		
	memcpy(wad->tag_data[index].data, data, size);

	/* Setup the tag data. */
	wad->tag_data[index].tag= type;
	wad->tag_data[index].length= size;
	wad->tag_data[index].offset= offset;

	return wad;
}

void remove_tag_from_wad(
	struct wad_data *wad, 
	WadDataType type)
{
	short index;

	assert(wad);
	assert(!wad->read_only_data);

	/* Find the index to replace */
	for(index= 0; index<wad->tag_count; ++index)
	{
		if(wad->tag_data[index].tag==type) break;
	}
	
	/* If we are appending... */
	if(index!=wad->tag_count)
	{
		struct tag_data *old_data= wad->tag_data;

		wad->tag_count-= 1;
		wad->tag_data= (struct tag_data *) malloc(wad->tag_count*sizeof(struct tag_data));
		
		if(!wad->tag_data)
		{
			alert_user(fatalError, strERRORS, outOfMemory, memory_error());
		}

		assert(wad->tag_data);
		memset(wad->tag_data, 0, wad->tag_count*sizeof(struct tag_data));
		if(old_data)
		{
			/* Copy the stuff below it. */
			memcpy(wad->tag_data, old_data, index*sizeof(struct tag_data));
			
			/* Copy the stuff above it. */
			memcpy(&wad->tag_data[index], &old_data[index+1], (wad->tag_count-index)*sizeof(struct tag_data));
			
			free(old_data);
		}
	}

	return;
}

/* Now uses CRC to checksum.. */
void calculate_and_store_wadfile_checksum(
	fileref file_id)
{
	struct wad_header header;
	
	/* read the header */
	read_wad_header(file_id, &header);

	/* Make sure we don't checksum the checksum value.. */
	header.checksum= 0l;
	write_wad_header(file_id, &header);
	
	/* Unused bytes better ALWAYS be initialized to zero.. */
	header.checksum= calculate_crc_for_opened_file(file_id);

	/* Save it.. */
	write_wad_header(file_id, &header);
}

boolean write_wad(
	fileref file_id, 
	struct wad_header *file_header,
	struct wad_data *wad, 
	long offset)
{
	FileError error= 0;
	boolean success;
	short entry_header_length= get_entry_header_length(file_header);
	short index;
	struct entry_header header;
	long running_offset= 0l;

	assert(wad);
	assert(!wad->read_only_data);

	for(index=0; !error && index<wad->tag_count; ++index)
	{
		header.tag= wad->tag_data[index].tag;
		header.length= wad->tag_data[index].length;

		/* On older versions, this will get overwritten by the copy.. */
		header.offset= wad->tag_data[index].offset;
	
		if(index==wad->tag_count-1)
		{
			/* Last one's next offset is zero.. */
			header.next_offset= 0;
		} else {
			running_offset+= header.length+entry_header_length;
			header.next_offset= running_offset;
		}

		/* Write this to the file... */
		error= write_to_file(file_id, offset, &header, entry_header_length);
		if(!error)
		{
			offset+= entry_header_length;
		
			/* Write the data.. */
			error= write_to_file(file_id, offset, wad->tag_data[index].data, wad->tag_data[index].length);
			if(!error)
			{
				offset+= wad->tag_data[index].length;
			}
		}
	}
	
	if(error)
	{
		success= FALSE;
		set_game_error(systemError, error);
	} else {
		success= TRUE;
	}
	
	return success;
}

short number_of_wads_in_file(
	FileDesc *file)
{
	fileref file_ref;
	short count= NONE;
	
	file_ref= open_wad_file_for_reading(file);
	if(file_ref>=0)
	{
		struct wad_header header;
		
		/* read the header */
		read_wad_header(file_ref, &header);
		
		count= header.wad_count;
		
		close_wad_file(file_ref);
	}
	
	return count;
}

void free_wad(
	struct wad_data *wad)
{
	short ii;
	
	assert(wad);
	
	/* Free all of the tags */
	if(wad->read_only_data)
	{
		/* Read only wad.. */
		free(wad->read_only_data);
		free(wad->tag_data);
	} else {
		/* Modifiable */
		for(ii=0; ii<wad->tag_count; ++ii)
		{
			assert(wad->tag_data[ii].data);
			free(wad->tag_data[ii].data);
		}
		free(wad->tag_data);
	}
	
	/* And free the total data.. */
	free(wad);
}

long calculate_wad_length(
	struct wad_header *file_header, 
	struct wad_data *wad)
{
	short ii;
	short header_length= get_entry_header_length(file_header);
	long running_length= 0l;

	for(ii= 0; ii<wad->tag_count; ++ii)
	{
		running_length += wad->tag_data[ii].length + header_length;
	}
	
	return running_length;
}

/* ------------ Transfer type functions */
#define CURRENT_FLAT_MAGIC_COOKIE (0xDEADDEAD)
struct encapsulated_wad_data {
	long magic_cookie;			/* Simple version control */
	long length;				/* Length, including everything.. */
	struct wad_header header;	/* The 128 byte header */
	/* Flat wad.. */
};

void *get_flat_data(
	FileDesc *base_file, 
	boolean use_union, 
	short wad_index)
{
	fileref file_handle;
	struct wad_header header;
	boolean success= FALSE;
	struct encapsulated_wad_data *data= NULL;
	
	assert(!use_union);

	file_handle= open_wad_file_for_reading(base_file);
	if(file_handle>=0)
	{
		/* Read the file */
		success= read_wad_header(file_handle, &header);

		if (success)
		{
			long length;
			FileError error= 0;
		
			/* Allocate the conglomerate data.. */
			error= size_of_indexed_wad(file_handle, &header, wad_index, &length);
			if(!error)
			{
				data= (struct encapsulated_wad_data *) malloc(length+sizeof(struct encapsulated_wad_data));
				if(data)
				{
					byte *buffer= ((byte *) data)+sizeof(struct encapsulated_wad_data);
				
					data->magic_cookie= CURRENT_FLAT_MAGIC_COOKIE;
					data->length= length+sizeof(struct encapsulated_wad_data);
					memcpy(&data->header, &header, sizeof(struct wad_header));

					/* Read into our buffer... */
					error= read_indexed_wad_from_file_into_buffer(file_handle, &header, wad_index, 
						buffer, &length);

					if(error)
					{
						/* Error-> didn't get it.. */
						free(data);
						data= (struct encapsulated_wad_data *) NULL;
					}
				} 
				else 
				{
					error= memory_error();
				}
			}

			set_game_error(systemError, error);
		}
		
		/* Close the file.. */
		close_wad_file(file_handle);
	}

	return data;
}

long get_flat_data_length(
	void *data)
{
	struct encapsulated_wad_data *d= (struct encapsulated_wad_data *) data;
	return d->length;
}

/* This is how you dispose of it-> you inflate it, then use free_wad() */
struct wad_data *inflate_flat_data(
	void *data, 
	struct wad_header *header)
{
	struct encapsulated_wad_data *d= (struct encapsulated_wad_data *) data;
	struct wad_data *wad= NULL;
	byte *buffer= ((byte *) data)+sizeof(struct encapsulated_wad_data);
	long raw_length;

	assert(data);
	assert(header);
	assert(d->magic_cookie==CURRENT_FLAT_MAGIC_COOKIE);

	memcpy(header, &(d->header), sizeof(struct wad_header));

	raw_length= calculate_raw_wad_length(header, buffer);
	assert(raw_length==d->length-sizeof(struct encapsulated_wad_data));
	
	/* Now inflate.. */
	wad= (struct wad_data *)convert_wad_from_raw(header, (byte *)data, sizeof(struct encapsulated_wad_data), raw_length);
	
	return wad;
}

/* ---------- debugging routines. */
void dump_wad(
	struct wad_data *wad)
{
	short index;
	struct tag_data *tag= wad->tag_data;

	dprintf("---Dumping---;g");
	dprintf("Tag Count: %d;g", wad->tag_count);
	for(index= 0; index<wad->tag_count; ++index)
	{
		assert(tag);
		dprintf("Tag: %x data: %x length: %d offset: %d;g", tag->tag, tag->data, tag->length,
			tag->offset);
		tag++;
	}
	dprintf("---End of Dump---");
}

/* ---------- file management routines */
FileError create_wadfile(
	FileDesc *file,
	unsigned long file_type) /* Should be OSType, or extension for dos. */
{
	return create_file(file, file_type);
}

fileref open_wad_file_for_reading(
	FileDesc *file)
{
	return open_file_for_reading(file);
}

fileref open_wad_file_for_writing(
	FileDesc *file)
{
	return open_file_for_writing(file);
}

void close_wad_file(
	fileref file_id)
{
	if(file_id>=0) /* It is a real file descriptor */
	{
		close_file(file_id);
	} else { /* It is the index of the internal data -1 */
		free_internal_data_for_file_id(file_id);
	}
	
	return;
}

/* ------------------------------ Private Code --------------- */
static FileError size_of_indexed_wad(
	fileref file_id, 
	struct wad_header *header, 
	short index, 
	long *length)
{
	struct directory_entry entry;
	FileError error;
	
	assert(file_id>=0); /* No union wads! */
	
	error= read_indexed_directory_data(file_id, header, index, &entry);
	if(!error)
	{
		*length= entry.length;
	}
	
	return error;
}

static long calculate_directory_offset(
	struct wad_header *header, 
	short index)
{
	long offset;
	long unit_size;
	long additional_offset;

	switch(header->version)
	{
		case PRE_ENTRY_POINT_WADFILE_VERSION:
			assert(header->application_specific_directory_data_size==0);
		case WADFILE_HAS_DIRECTORY_ENTRY:
		case WADFILE_SUPPORTS_OVERLAYS:
		case INFINITY_WADFILE_VERSION:
			assert(header->application_specific_directory_data_size>=0);
			unit_size= header->application_specific_directory_data_size+get_directory_base_length(header);
			additional_offset= header->application_specific_directory_data_size;
			break;
			
		default:
			vhalt(csprintf(temporary, "what is version %d?", header->version));
			break;
	}

	/* Now actually calculate it (Note that the directory_entry data is first) */
	offset= header->directory_offset+(index*unit_size);
	
	return offset;
}

static short get_entry_header_length(
	struct wad_header *header)
{
	short size;

	assert(header);
	
	switch(header->version)
	{
		case PRE_ENTRY_POINT_WADFILE_VERSION:
		case WADFILE_HAS_DIRECTORY_ENTRY:
			size = sizeof(struct old_entry_header);
			break;

		default:
			/* After this point, I stored it. */
			size = header->entry_header_size;
			break;
	}
	
	return size;
}

static short get_directory_base_length(
	struct wad_header *header)
{
	short size;
	
	assert(header);
	assert(header->version<=CURRENT_WADFILE_VERSION || header->version==INFINITY_WADFILE_VERSION);

	switch(header->version)
	{
		case PRE_ENTRY_POINT_WADFILE_VERSION:
		case WADFILE_HAS_DIRECTORY_ENTRY:
			size = sizeof(struct old_directory_entry);
			break;

		default:
			/* After this point, I stored it. */
			size = header->directory_entry_base_size;
			break;
	}
		
	return size;
}

/* This searches the directories for the given index, to allow for special replacements. */
static FileError read_indexed_directory_data(
	fileref ref,
	struct wad_header *header,
	short index,
	struct directory_entry *entry)
{
	short base_entry_size;
	long offset;
	FileError error= 0;

	/* Get the sizes of the data structures */
	base_entry_size= get_directory_base_length(header);
	
	/* For old files, the index==the actual index */
	if(header->version<=WADFILE_HAS_DIRECTORY_ENTRY) 
	{
		/* Calculate the offset */
		offset= calculate_directory_offset(header, index);

		/* Read it! */
		assert(base_entry_size<=sizeof(struct directory_entry));
		error= read_from_file(ref, offset, entry, base_entry_size);
	} else {
		short directory_index;

		/* Pin it, so we can try to read future file formats */
		if(base_entry_size>sizeof(struct directory_entry)) 
		{
			base_entry_size= sizeof(struct directory_entry);
		}
	
		/* We have to loop.. */
		for(directory_index= 0; !error && directory_index<header->wad_count; ++directory_index)
		{
			/* We use a hint, that the index is the real index, to help make this have */
			/* a “hit” on the first try */
			short test_index= (index+directory_index)%header->wad_count;
		
			/* Calculate the offset */
			offset= calculate_directory_offset(header, test_index);

			/* Read it.. */
			error= read_from_file(ref, offset, entry, base_entry_size);
			if(entry->index==index) 
			{
				break; /* Got it! */
			}
		}
	}

	/* File error has precedence. */
	return error;
}

/* Internal function.. */
static FileError read_indexed_wad_from_file_into_buffer(
	fileref file_id, 
	struct wad_header *header, 
	short index,
	void *buffer,
	long *length) /* Length of maximum buffer on entry, actual length on return */
{
	struct directory_entry entry;
	FileError error= 0;

	/* No union files for this one.. */
	assert(file_id>=0);

	/* Read the directory entry first */
	error= read_indexed_directory_data(file_id, header, index, &entry);
	if(!error)
	{
		/* Check the index stored in the entry, and assert if they don't match! */
		assert((get_directory_base_length(header)==sizeof(struct old_directory_entry)) || entry.index==index);
		assert(*length<=entry.length);
		assert(buffer);

		/* Read into it. */
		error= read_from_file(file_id, entry.offset_to_start, buffer, entry.length);
			
		/* Set the length */
		*length= entry.length;

		/* Veracity Check */
		/* ! an error, it has a length non-zero and calculated != actual */
		assert(entry.length==calculate_raw_wad_length(header, (byte *)buffer));
	}
	
	return error;
}

/* This *MUST* be a base wad.. */
static struct wad_data *convert_wad_from_raw(
	struct wad_header *header, 
	byte *data,
	long wad_start_offset,
	long raw_length)
{
	struct wad_data *wad;
	byte *raw_wad;
	
	/* In case we are somewhere else, like, for example, in a net transferred level.. */
	raw_wad= data+wad_start_offset;
	
	wad= (struct wad_data *) malloc(sizeof(struct wad_data));
	if(wad)
	{
		short tag_count;

		/* Clear it */
		memset(wad, 0, sizeof(struct wad_data));

		/* If the wad is of non-zero length... */
		if(raw_length) 
		{
			/* Count the tags */
			tag_count= count_raw_tags(raw_wad);
	
			/* Allocate the tags.. */
			wad->tag_count= tag_count;
			wad->tag_data= (struct tag_data *) malloc(tag_count * sizeof(struct tag_data));
			if(wad->tag_data)
			{
				struct entry_header *wad_entry_header;
				short index;
				short entry_header_size;
			
				/* Clear it */
				memset(wad->tag_data, 0, tag_count*sizeof(struct tag_data));
				
				entry_header_size= get_entry_header_length(header);
				wad_entry_header= (struct entry_header *) raw_wad;

				/* Note that this is a read only wad.. */	
				wad->read_only_data= data;
	
				for(index= 0; index<tag_count; ++index)
				{
					assert(header->version<WADFILE_SUPPORTS_OVERLAYS || wad_entry_header->offset==0l);
					wad->tag_data[index].tag= wad_entry_header->tag;
					wad->tag_data[index].length= wad_entry_header->length;
					wad->tag_data[index].offset= 0l;
					wad->tag_data[index].data= ((byte *) wad_entry_header)+entry_header_size;
					wad_entry_header= (struct entry_header *) (raw_wad + wad_entry_header->next_offset);
#ifdef OBSOLETE					
					if(wad->tag_data[index].data)
					{
						/* This MUST be a base! */
						assert(header->version<WADFILE_SUPPORTS_OVERLAYS || wad_entry_header->offset==0l);
		
						/* Copy the data.. */
						memcpy(wad->tag_data[index].data, 
							(((byte *) wad_entry_header)+entry_header_size), 
							wad_entry_header->length);
						wad_entry_header= (struct entry_header *) (raw_wad + wad_entry_header->next_offset);
					} else {
						alert_user(fatalError, strERRORS, outOfMemory, memory_error());
					}
#endif
				} 
			} else {
				alert_user(fatalError, strERRORS, outOfMemory, memory_error());
			}
		}
	}
	
	return wad;
}

/* This *MUST* be a base wad.. */
static struct wad_data *convert_wad_from_raw_modifiable(
	struct wad_header *header, 
	byte *raw_wad,
	long raw_length)
{
	struct wad_data *wad;
	
	wad= (struct wad_data *) malloc(sizeof(struct wad_data));
	if(wad)
	{
		short tag_count;

		/* Clear it */
		memset(wad, 0, sizeof(struct wad_data));

		/* If the wad is of non-zero length... */
		if(raw_length) 
		{
			/* Count the tags */
			tag_count= count_raw_tags(raw_wad);
	
			/* Allocate the tags.. */
			wad->tag_count= tag_count;
			wad->tag_data= (struct tag_data *) malloc(tag_count * sizeof(struct tag_data));
			if(wad->tag_data)
			{
				struct entry_header *wad_entry_header;
				short index;
				short entry_header_size;
			
				/* Clear it */
				memset(wad->tag_data, 0, tag_count*sizeof(struct tag_data));
				
				entry_header_size= get_entry_header_length(header);
				wad_entry_header= (struct entry_header *) raw_wad;
	
				for(index= 0; index<tag_count; ++index)
				{
					wad->tag_data[index].tag= wad_entry_header->tag;
					wad->tag_data[index].length= wad_entry_header->length;
					wad->tag_data[index].data= (byte *) malloc(wad_entry_header->length);
					if(!wad->tag_data[index].data)
					{
						alert_user(fatalError, strERRORS, outOfMemory, memory_error());
					}
					wad->tag_data[index].offset= 0l;
					
					/* This MUST be a base! */
					assert(header->version<WADFILE_SUPPORTS_OVERLAYS || wad_entry_header->offset==0l);
	
					/* Copy the data.. */
					memcpy(wad->tag_data[index].data, 
						(((byte *) wad_entry_header)+entry_header_size), 
						wad_entry_header->length);
					wad_entry_header= (struct entry_header *) (raw_wad + wad_entry_header->next_offset);
				} 
			}
		}
	}
	
	return wad;
}

static short count_raw_tags(
	byte *raw_wad)
{
	short tag_count= 1;
	struct entry_header *wad_entry_header;
		
	wad_entry_header= (struct entry_header *) raw_wad;

	while (wad_entry_header->next_offset)	
	{	
		tag_count++;
		wad_entry_header= (struct entry_header *) (raw_wad + wad_entry_header->next_offset);
	} 

	return tag_count;
}

/* Patch it! */
static void patch_wad_from_raw(
	struct wad_header *header, 
	byte *raw_wad, 
	struct wad_data *read_wad)
{
	short tag_count;
	struct entry_header *wad_entry_header;
	short index;
	short entry_header_size= get_entry_header_length(header);
	
	/* Count the tags */
	tag_count= count_raw_tags(raw_wad);

	/* On patch file.. */
	assert(header->version>= WADFILE_SUPPORTS_OVERLAYS);
	wad_entry_header= (struct entry_header *) raw_wad;
	for(index= 0; index<tag_count; ++index)
	{
		short actual_index;
		
		for(actual_index=0; actual_index<read_wad->tag_count; ++actual_index)
		{
			if(read_wad->tag_data[actual_index].tag==wad_entry_header->tag)
			{
				/* MATCH! */
				assert(wad_entry_header->offset+wad_entry_header->length<=
					read_wad->tag_data[actual_index].length);
				
				/* Copy it in.. */
				memcpy(read_wad->tag_data[actual_index].data+wad_entry_header->offset,
					(((byte *) wad_entry_header)+entry_header_size),
					wad_entry_header->length);
				
				break;
			}
		}

		if(actual_index==read_wad->tag_count)
		{
			/* This is an overload of a tag that wasn't previously existant for this level... */
			dprintf("New tags are not supported!");
		}
		
		wad_entry_header= (struct entry_header *) (raw_wad + wad_entry_header->next_offset);
	} 
}

static long calculate_raw_wad_length(
	struct wad_header *file_header,
	byte *wad)
{
	struct entry_header *header;
	long length;
	short entry_header_size;
	
	entry_header_size= get_entry_header_length(file_header);
	
	length= 0;
	header= (struct entry_header *) wad;
	while(header->next_offset)
	{
		length += header->length + entry_header_size;
		header= (struct entry_header *) ((byte *) wad + header->next_offset);
	}
	length += header->length + entry_header_size;

	return length;
}

/* -------- debug privates */
static void dump_raw_wad(
	byte *wad)
{
	struct entry_header *header;
	long offset;
	short tag_count;

	assert(wad);
	
	offset= tag_count= 0;
	header= (struct entry_header *) wad;
	
	while(header->next_offset)
	{
		offset = header->next_offset;
		dprintf("%d Tag: %x Length: %d Next Offset: %d", tag_count, header->tag, header->length, header->next_offset);
		tag_count++;
		header= (struct entry_header *) (((byte *) wad) + offset);
	}
	dprintf("%d Tag: %x Length: %d Next Offset: %d", tag_count, header->tag, header->length, header->next_offset);
}

static void dump_union_ref(
	fileref union_ref)
{
	struct wad_internal_data *data= get_internal_data_for_file_id(union_ref);
	short ii;
	
	assert(data);

	if(data->file_count>1)
	{
		dprintf("Dump for union ref: %d", union_ref);
		for(ii= 0; ii<data->file_count; ++ii)
		{
			dprintf("File %d VRef: %d parID: %d Name: %P", ii, data->files[ii].vRefNum,
				data->files[ii].parID, data->files[ii].name);
		}
		dprintf("End Dump for union ref: %d", union_ref);
	}
}

/* -------- union static functions */
static struct wad_internal_data *get_internal_data_for_file_id(
	short file_id)
{
	short actual_index= (-file_id)-1;

	assert(actual_index>=0 && actual_index<MAXIMUM_OPEN_WADFILES);
	return internal_data[(-file_id)-1];
}

static struct wad_internal_data *allocate_internal_data_for_file_id(
	short file_id)
{
	short actual_index= (-file_id)-1;

	assert(actual_index>=0 && actual_index<MAXIMUM_OPEN_WADFILES);

	/* Make sure it isn't already allocated */
	assert(!internal_data[actual_index]);

	internal_data[actual_index]= (struct wad_internal_data *)malloc(sizeof(struct wad_internal_data));
	if(!internal_data[actual_index]) alert_user(fatalError, strERRORS, outOfMemory, memory_error());

	/* If we got it.. */
	if(internal_data[actual_index])
		internal_data[actual_index]->file_count= 0;
	
	return internal_data[actual_index];
}

/* Not static because mac_wad.c needs it. (gross hack) */
void free_internal_data_for_file_id(
	short file_id)
{
	short actual_index= (-file_id)-1;

	assert(actual_index>=0 && actual_index<MAXIMUM_OPEN_WADFILES);

	/* Make sure it is already allocated */
	assert(internal_data[actual_index]);
	free(internal_data[actual_index]);
	internal_data[actual_index]= NULL;
}

static fileref find_available_union_refnum(
	void)
{
	short i;
	fileref ref;
	
	for(i= 0; i<MAXIMUM_OPEN_WADFILES; ++i) 
	{
		if(!internal_data[i]) break;
	}
	
	if(i==MAXIMUM_OPEN_WADFILES) 
	{
		/* No more files! */
		ref= 0;
	} else {
		/* Got one! */
		ref= -1 - i;
	}
	
	return ref;
}

static FileError write_to_file(
	fileref file_id, 
	long offset, 
	void *data, 
	long length)
{
	FileError err;

	assert(file_id != -1);
	err= set_fpos(file_id, offset);
	if (!err)
	{
		err= write_file(file_id, length,  data);
		assert(!err);
	}
	
	return err;
}

static FileError read_from_file(
	fileref file_id, 
	long offset, 
	void *data, 
	long length)
{
	FileError err;

	assert(file_id != -1);
	err= set_fpos(file_id, offset);
	if (!err)
	{
		err= read_file(file_id, length, data);
	}
	
	return err;
}

