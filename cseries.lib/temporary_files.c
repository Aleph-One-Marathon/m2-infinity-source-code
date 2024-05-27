/*
TEMPORARY_FILES.C
Thursday, March 25, 1993 6:04:38 PM

Thursday, March 25, 1993 6:49:01 PM
	this is a rather impoverished implementation.
Friday, April 16, 1993 9:43:19 AM
	we replace the temporary file, if it exists.  very sound.
*/

#include "macintosh_cseries.h"
#include "temporary_files.h"

#include <Folders.h>

#ifdef mpwc
#pragma segment modules
#endif

/* ---------- constants */

#define TEMPORARY_FILENAME "\pGunfighter’s Amnesia"

/* ---------- structures */

/* ---------- globals */

/* ---------- code */

OSErr OpenTemporaryFile(
	short *tempRefNum)
{
	short tempVRefNum;
	long tempDirID;
	OSErr error;
	
	/* check for the temporary folder using FindFolder, creating it if necessary */
	FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &tempVRefNum, &tempDirID);

	error= HCreate(tempVRefNum, tempDirID, TEMPORARY_FILENAME, '\?\?\?\?', 'TEMP');
	if (error==noErr||error==dupFNErr)
	{
		error= HOpen(tempVRefNum, tempDirID, TEMPORARY_FILENAME, fsRdWrPerm, tempRefNum);
	}
	
	return error;
}

OSErr CloseTemporaryFile(
	short tempRefNum)
{
	short tempVRefNum;
	long tempDirID;
	OSErr error;
	
	error= FSClose(tempRefNum);
	if (error==noErr)
	{
		/* check for the temporary folder using FindFolder, creating it if necessary */
		FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &tempVRefNum, &tempDirID);
	
		error= HDelete(tempVRefNum, tempDirID, TEMPORARY_FILENAME);
	}
	
	return error;
}
