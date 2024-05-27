/*
IMPORT_DEFINITIONS.C
Sunday, October 2, 1994 1:25:23 PM  (Jason')
*/

#include "macintosh_cseries.h"

#include "interface.h"
#include "shell.h"
#include "definitions.h"

/* ---------- globals */

/* sadly extern'ed from their respective files */
extern byte monster_definitions[];
extern byte projectile_definitions[];
extern byte effect_definitions[];
extern byte weapon_definitions[];
extern byte physics_models[];

/* ---------- code */

void import_definition_structures(
	void)
{
	short refNum;
	OSErr error= FSOpen(getpstr(temporary, strFILENAMES, filenamePHYSICS_MODEL), 0, &refNum);
	
	if (error==noErr)
	{
		/* warn the user that external physics models are Bad Thing™ */
		alert_user(infoError, strERRORS, warningExternalPhysicsModel, 0);
		
		/* until we get an error (e.g., EOF), fetch definition_data structures, stuffing
			them where they belong if we recognize the tag */
		do
		{
			long count;
			struct definition_data data;
			
			count= sizeof(struct definition_data);
			error= FSRead(refNum, &count, &data);
			if (error==noErr)
			{
				void *buffer= (void *) NULL;
				
				switch (data.tag)
				{
					case MONSTER_TAG: buffer= monster_definitions; break;
					case EFFECT_TAG: buffer= effect_definitions; break;
					case PROJECTILE_TAG: buffer= projectile_definitions; break;
					case PHYSICS_TAG: buffer= physics_models; break;
					case WEAPON_TAG: buffer= weapon_definitions; break;
				}
				
				count= data.count*data.size;
				if (buffer)
				{
					error= FSRead(refNum, &count, buffer);
				}
				else
				{
					error= SetFPos(refNum, fsFromMark, count);
				}
			}
		}
		while (error==noErr);
		
		FSClose(refNum);
	}
	
	return;
}
