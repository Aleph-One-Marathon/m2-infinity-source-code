/*

	wad_macintosh.c
	Saturday, August 26, 1995 2:04:59 PM- rdm created.

	routines from wad.c that are not portable!
*/

#include "macintosh_cseries.h"
#include "wad.h"
#include "tags.h"
#include "game_errors.h"

#include "find_files.h"

struct find_files_private_data { /* used for enumerating wadfiles */
	FileDesc *base_file;
	long base_checksum;
};

static Boolean checksum_and_not_base_callback(FSSpec *file, void *data);

/* ---- code */
/*  altogether. */
/* Find other entries that reference the base wad.  Search is by type, and when one is found, the */
/*  checksums are checked, to make sure that it is modifying the correct file. */
FileError find_other_entries_that_reference_checksum(
	unsigned long checksum,
	FileDesc *files_array,
	short *count)
{
	struct find_file_pb pb;
	OSType file_type;
	struct find_files_private_data private_data;
	FInfo file_info;
	FileError error;

	/* What type of file are we? */
	FSpGetFInfo((FSSpec *) &files_array[0], &file_info);
	file_type= file_info.fdType;

	/* Setup the private data for the callback */
	private_data.base_file= &files_array[0];
	private_data.base_checksum= checksum;
	
	/* Clear out the find_file pb */
	memset(&pb, 0, sizeof(struct find_file_pb));
	
	/* Set the information */
	pb.version= 0;
	pb.flags= _ff_recurse;
	pb.vRefNum= files_array[0].vRefNum;
	pb.directory_id= files_array[0].parID;
	pb.type_to_find= file_type;
	pb.buffer= (FSSpec *) &files_array[1]; /* First one is already set.. */
	pb.max= MAXIMUM_UNION_WADFILES-1;
	pb.callback= checksum_and_not_base_callback;
	pb.user_data= &private_data;

	/* Find them! */
	error= find_files(&pb);
	
	if(!error) 
	{
		*count= pb.count+1; /* +1 because base is already added. */
	} else {
		*count= 1;
	}

	return error;
}

static Boolean checksum_and_not_base_callback(
	FSSpec *file, 
	void *data)
{
	Boolean add_this_file= FALSE;
	struct find_files_private_data *private= (struct find_files_private_data *) data;
	
	/* Don't readd the base file.. */
	if(!equal_fsspecs(file, (FSSpec *) private->base_file))
	{
		/* Do the checksums match? */
		if(wad_file_has_parent_checksum((FileDesc *) file, private->base_checksum))
		{
			add_this_file= TRUE;
		}
	}
	
	return add_this_file;
}