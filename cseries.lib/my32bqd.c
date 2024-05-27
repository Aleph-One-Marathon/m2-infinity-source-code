/*
MY32BQD.C
Saturday, January 11, 1992 7:31:22 PM

Saturday, April 11, 1992 1:03:18 PM
	finished (started); dead code available for eventually writing UpdateGWorld.
Thursday, April 16, 1992 8:25:48 AM
	my substitution for NewGWorld was calling SetGWorld and GetGWorld-- a pretty stupid idea.
Tuesday, August 18, 1992 7:25:35 PM
	myNewGWorld now calls GetMaxDevice() with the given global rectangle instead of always
	assuming we’re using the main device.
Wednesday, August 19, 1992 11:05:03 AM
	coded myUpdateGWorld, probably unnecessairly (DisposeGWorld,NewGWorld works fine).
Friday, April 9, 1993 11:49:08 AM
	the memory on the 8•24 GC board is slow, at least when accessed from the 68k, so we
	now make an effort to keep our pixels local.  does this work under the old 32BQD?
Tuesday, November 9, 1993 11:13:34 AM
	uhm.  no.  moron.  removed keepLocal flag from system 6 32BQD NewGWorld calls.  added
	more error-checking to the non-32BQD case.  there exist bugs, somewhere.
*/

#include "macintosh_cseries.h"
#include "my32bqd.h"

#ifdef mpwc
#pragma segment my32bqd
#endif

/* ---------- constants */

#define QD32Trap 0xab03
#define UnImplTrap 0xa89f

#define TAG 'myGW'

/* ---------- enumerated types */

enum
{
	UnknownQD,
	SystemSixQD,
	SystemSix32BQD,
	SystemSeven32BQD
};

/* ---------- structures */

struct myGWorld
{
	CGrafPort port;
	long tag;
	GDHandle device;
	
	Handle pixels;
	boolean locked;
};
typedef struct myGWorld *myGWorldPtr;

/* ---------- macros */

#define IS_MYGWORLD(port) (TAG==*((long*)((CGrafPtr)(port+1))))

/* ---------- globals */

static short offscreen_type= UnknownQD;
static OSErr myQDErrorValue;

/* ---------- code */

void initialize_my_32bqd(
	void)
{
	SysEnvRec environment;

#ifdef env68k
	if (NGetTrapAddress(QD32Trap, ToolTrap)==NGetTrapAddress(UnImplTrap, ToolTrap))
	{
		offscreen_type= SystemSixQD;
	}
	else
	{
		if (SysEnvirons(curSysEnvVers, &environment)==noErr)
		{
			offscreen_type= (environment.systemVersion<0x0700) ? SystemSix32BQD : SystemSeven32BQD;
		}
		else
		{
			offscreen_type= SystemSixQD;
		}
	}
#else
	offscreen_type= SystemSeven32BQD;
#endif
	
	return;
}

/* we only support the PixelDepth==0 mode, though we now handle multiple monitors */
QDErr myNewGWorld(
	GWorldPtr *offscreenGWorld,
	short PixelDepth,
	const Rect *boundsRect,
    CTabHandle cTable,
	GDHandle aGDevice,
	GWorldFlags flags)
{
	GDHandle device, intersected_device, old_device;
	PixMapHandle pixmap;
	CGrafPtr port, old_port;
	Handle pixels;
	myGWorldPtr world;
	QDErr error;
	Rect newBounds;
	
#ifndef DEBUG
	#pragma unused (flags)
#endif

	assert(!PixelDepth&&!cTable&&!flags);

	intersected_device= GetMaxDevice(boundsRect);
	assert(intersected_device);

	switch (offscreen_type)
	{
		case SystemSixQD:
			assert((*(*intersected_device)->gdPMap)->pixelSize==8);

			newBounds= *boundsRect;
			OffsetRect(&newBounds, -newBounds.left, -newBounds.top);
			
			pixels= NewHandle((long)newBounds.right*(long)newBounds.bottom);
			world= (myGWorldPtr) NewPtr(sizeof(struct myGWorld));
			if (world&&pixels)
			{
				world->device= NewGDevice(NULL, -1);
				if (world->device)
				{
					/* initialize our device */
					device= world->device;
					
					(*device)->gdID= 0;
					(*device)->gdType= clutType;
					(*device)->gdResPref= 3;
					(*device)->gdSearchProc= NULL;
					(*device)->gdCompProc= NULL;
					(*device)->gdRect= newBounds;
					(*device)->gdFlags= (1<<gdDevType) | (1<<ramInit) | (1<<noDriver);
					
					/* initialize the pixmap */
					pixmap= (*device)->gdPMap;
					(*pixmap)->baseAddr= NULL;
					(*pixmap)->bounds= newBounds;
					(*pixmap)->rowBytes= 0x8000 | (*pixmap)->bounds.right;
					(*pixmap)->pixelSize= 8;
					(*pixmap)->cmpCount= 1;
					(*pixmap)->cmpSize= 8;
					
					/* install a new color table from the intersected device */
					DisposCTable((*pixmap)->pmTable);
					(*pixmap)->pmTable= (*(*intersected_device)->gdPMap)->pmTable;
					
					error= HandToHand((Handle *)&(*pixmap)->pmTable);
					if (error==noErr)
					{
						(*(*pixmap)->pmTable)->ctFlags= 0;
												
						MakeITable((*pixmap)->pmTable, (*device)->gdITable, 3);

						myGetGWorld(&old_port, &old_device);
						SetGDevice(device);

						/* initialize the cgrafport */
						port= &world->port;
						OpenCPort(port);
						RectRgn(port->visRgn, &newBounds);
						port->portRect= newBounds;
						
						world->locked= FALSE;
						world->pixels= pixels;
						
						*offscreenGWorld= (GWorldPtr) world;
						mySetGWorld(old_port, old_device);
					}
				}
				else
				{
					error= MemError();
				}
			}
			else
			{
				error= MemError();
			}
			myQDErrorValue= error;
			break;
		
		case SystemSix32BQD:
			error= NewGWorld(offscreenGWorld, PixelDepth, boundsRect, cTable, aGDevice, 0);
			break;
		case SystemSeven32BQD:
			error= NewGWorld(offscreenGWorld, PixelDepth, boundsRect, cTable, aGDevice, keepLocal);
			break;
		
		default:
			halt();
	}
	
	return error;
}

void myDisposeGWorld(
	GWorldPtr offscreenGWorld)
{
	switch(offscreen_type)
	{
		case SystemSixQD:
			DisposGDevice(((myGWorldPtr)offscreenGWorld)->device);
			DisposeHandle(((myGWorldPtr)offscreenGWorld)->pixels);
			DisposePtr((Ptr)offscreenGWorld);
			myQDErrorValue= noErr;
			break;
		
		case SystemSix32BQD:
		case SystemSeven32BQD:
			DisposeGWorld(offscreenGWorld);
			break;
		
		default:
			halt();
	}
	
	return;
}

Boolean myLockPixels(
	GWorldPtr world)
{
	myGWorldPtr myworld= (myGWorldPtr) world;
	
	switch(offscreen_type)
	{
		case SystemSixQD:
			assert(!myworld->locked);
			myworld->locked= TRUE;
			HLock(myworld->pixels);
			(*(*myworld->device)->gdPMap)->baseAddr= StripAddress(*(myworld->pixels));
			(*myworld->port.portPixMap)->baseAddr= StripAddress(*(myworld->pixels));
			return TRUE;

		case SystemSix32BQD:
			return LockPixels(world->portPixMap);
		
		case SystemSeven32BQD:
			return LockPixels(GetGWorldPixMap(world));
		
		default:
			halt();
	}
}

void myUnlockPixels(
	GWorldPtr world)
{
	myGWorldPtr myworld= (myGWorldPtr) world;
	
	switch(offscreen_type)
	{
		case SystemSixQD:
			assert(myworld->locked);
			myworld->locked= FALSE;
			HUnlock(myworld->pixels);
			(*(*myworld->device)->gdPMap)->baseAddr= NULL;
			break;
		
		case SystemSix32BQD:
			UnlockPixels(world->portPixMap);
			break;
		
		case SystemSeven32BQD:
			UnlockPixels(GetGWorldPixMap(world));
			break;
		
		default:
			halt();
	}
}

PixMapHandle myGetGWorldPixMap(
	GWorldPtr offscreenGWorld)
{
	switch(offscreen_type)
	{
		case SystemSixQD:
		case SystemSix32BQD:
			/* though these are two totally different structures, they both begin with a CGrafPort
				so this should work fine */
			return offscreenGWorld->portPixMap;
		
		case SystemSeven32BQD:
			return GetGWorldPixMap(offscreenGWorld);
		
		default:
			halt();
	}
}

Ptr myGetPixBaseAddr(
	GWorldPtr world)
{
	switch(offscreen_type)
	{
		case SystemSixQD:
			assert(((myGWorldPtr)world)->locked);
			return StripAddress(*(((myGWorldPtr)world)->pixels));
		
		case SystemSix32BQD:
			return GetPixBaseAddr(world->portPixMap);
			
		case SystemSeven32BQD:
			return GetPixBaseAddr(GetGWorldPixMap(world));
		
		default:
			halt();
	}
}

void myGetGWorld(
	CGrafPtr *port,
	GDHandle *gdh)
{
	switch(offscreen_type)
	{
		case SystemSixQD:
			GetPort((GrafPtr *)port);
			*gdh= GetGDevice();
			break;
		
		case SystemSix32BQD:
		case SystemSeven32BQD:
			GetGWorld(port, gdh);
			break;
		
		default:
			halt();
	}
	
	return;
}

OSErr myQDError(
	void)
{
	OSErr error;
	
	switch(offscreen_type)
	{
		case SystemSixQD:
			error= myQDErrorValue;
			break;
		
		default:
			error= QDError();
			break;
	}
	
	return error;
}

void mySetGWorld(
	CGrafPtr port,
	GDHandle gdh)
{
	switch(offscreen_type)
	{
		case SystemSixQD:
			if (IS_MYGWORLD(port))
			{
				SetPort((GrafPtr)port);
				SetGDevice(((myGWorldPtr)port)->device);
			}
			else
			{
				assert(gdh);
				SetPort((GrafPtr)port);
				SetGDevice(gdh);
			}
			break;
		
		case SystemSix32BQD:
		case SystemSeven32BQD:
			SetGWorld(port, gdh);
			break;
		
		default:
			halt();
	}
}

/* we don’t behave exactly like the real 32BQD here because it would try to align
	the pixmap for best copying performance, etc */
GWorldFlags myUpdateGWorld(
	GWorldPtr *offscreenGWorld,
	short pixelDepth,
    const Rect *boundsRect,
	CTabHandle cTable,
	GDHandle aGDevice,
	GWorldFlags flags)
{
	GDHandle intersected_device;
	CTabHandle intersected_color_table, world_color_table;
	PixMapHandle intersected_pixmap, world_pixmap;
	myGWorldPtr world= (myGWorldPtr) *offscreenGWorld;
	short exit_flags;
	Rect newBounds;

#ifndef DEBUG
	#pragma unused (flags)
#endif
	
	assert(!cTable&&!pixelDepth&&!flags);

	intersected_device= GetMaxDevice(boundsRect);
	assert(intersected_device);
	
	intersected_pixmap= (*intersected_device)->gdPMap;
	intersected_color_table= (*intersected_pixmap)->pmTable;

	exit_flags= 0;
	switch(offscreen_type)
	{
		case SystemSixQD:
			assert((*intersected_pixmap)->pixelSize==8);

			myQDErrorValue= noErr;
		
			world_pixmap= (*world->device)->gdPMap;
			world_color_table= (*world_pixmap)->pmTable;
	
			/* check bounds; reallocate if changed */
			newBounds= *boundsRect;
			OffsetRect(&newBounds, -newBounds.left, -newBounds.top);
			if (newBounds.right!=world->port.portRect.right||
				newBounds.bottom!=world->port.portRect.bottom)
			{
				SetHandleSize(world->pixels, newBounds.right*newBounds.bottom);
				if (MemError()!=noErr)
				{
					DisposeHandle(world->pixels);
					world->pixels= NewHandle(newBounds.right*newBounds.bottom);
					if (MemError()!=noErr)
					{
						myQDErrorValue= MemError();
						exit_flags|= gwFlagErr;
					}
				}
				
				if (myQDErrorValue==noErr)
				{
					(*world->device)->gdRect= newBounds;
					(*world_pixmap)->bounds= newBounds;
					(*world_pixmap)->rowBytes= 0x8000 | (*world_pixmap)->bounds.right;
					RectRgn(world->port.visRgn, &newBounds);
					world->port.portRect= newBounds;
					
					exit_flags|= reallocPix;
				}
			}

			if (myQDErrorValue==noErr)
			{
				/* check color table; bag old one if changed and copy in the new one */
				if ((*world_color_table)->ctSeed!=(*intersected_color_table)->ctSeed)
				{
					DisposCTable((*world_pixmap)->pmTable);
					(*world_pixmap)->pmTable= intersected_color_table;
					
					if (HandToHand((Handle *)&(*world_pixmap)->pmTable)==noErr)
					{
						world_color_table= (*world_pixmap)->pmTable;
						(*world_color_table)->ctFlags= 0;
												
						MakeITable(world_color_table, (*world->device)->gdITable, 3);
					}
					else
					{
						myQDErrorValue= MemError();
						exit_flags|= reallocPix;
					}
				}
			}
			
			break;
		
		case SystemSix32BQD:
			/* we’re fucked: system 6.0’s _UpdateGWorld doesn’t always work and we can’t
				use our own method-- so we bag and reallocate it */
			DisposeGWorld(*offscreenGWorld);
			if (NewGWorld(offscreenGWorld, pixelDepth, boundsRect, cTable, aGDevice, 0)!=noErr)
			{
				exit_flags= gwFlagErr;
			}
			break;
		
		case SystemSeven32BQD:
			exit_flags= UpdateGWorld(offscreenGWorld, pixelDepth, boundsRect, cTable, aGDevice, 0);
			break;
	}
	
	return exit_flags;
}
