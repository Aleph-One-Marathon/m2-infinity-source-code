/*
DEVICES.C
Wednesday, September 1, 1993 12:09:55 PM

Friday, August 12, 1994 10:23:19 AM
	added LowLevelSetEntries().
*/

#include "macintosh_cseries.h"
#include <Video.h>

#ifdef mpwc
#pragma segment modules
#endif

/* ---------- constants */

#define alrtCHANGE_DEPTH 130

/* ---------- globals */

/* for menu bar hiding */
static boolean menu_bar_hidden= FALSE;
static short old_menu_bar_height;
static RgnHandle old_gray_region;

/* ---------- private code */

static boolean parse_device(GDHandle device, short depth, GDHandle *changeable_device,
	short *new_mode, short *new_type);

/* ---------- code */

void HideMenuBar(
	GDHandle device)
{
	RgnHandle new_gray_region;
	
	assert(!menu_bar_hidden);
	
	if (device==GetMainDevice())
	{
		old_menu_bar_height= LMGetMBarHeight();
		LMSetMBarHeight(0);
		
		old_gray_region= LMGetGrayRgn();
		new_gray_region= NewRgn();
		RectRgn(new_gray_region, &(*device)->gdRect);
		LMSetGrayRgn(new_gray_region);
		
		menu_bar_hidden= TRUE;
	}
	
	return;
}

void ShowMenuBar(
	void)
{
	if (menu_bar_hidden)
	{
		LMSetMBarHeight(old_menu_bar_height);
		DisposeRgn(LMGetGrayRgn());
		LMSetGrayRgn(old_gray_region);
		DrawMenuBar();

		menu_bar_hidden= FALSE;
	}
	
	return;
}

GDHandle MostDevice(
	Rect *bounds)
{
	GDHandle device, largest_device;
	long largest_area, area;
	Rect intersection;
	
	largest_area= 0;
	largest_device= (GDHandle) NULL;
	for (device=GetDeviceList();device;device=GetNextDevice(device))
	{
		if (TestDeviceAttribute(device, screenDevice)&&TestDeviceAttribute(device, screenActive))
		{
			if (SectRect(bounds, &(*device)->gdRect, &intersection))
			{
				area= RECTANGLE_WIDTH(&intersection)*RECTANGLE_HEIGHT(&intersection);
				if (area>largest_area)
				{
					largest_area= area;
					largest_device= device;
				}
			}
		}
	}
	
	return largest_device;
}

GDHandle BestDevice(
	short depth)
{
	GDHandle device, changeable_device;
	short new_mode= 0, new_type= 0;
	OSErr error= noErr;

	/* use the main device, if it’s what we want */
	if (parse_device(GetMainDevice(), depth, &changeable_device, &new_mode, &new_type))
	{
		return GetMainDevice();
	}

	/* otherwise, go look for a device of the given depth */
	for (device=GetDeviceList();device;device=GetNextDevice(device))
	{
		if (parse_device(device, depth, &changeable_device, &new_mode, &new_type))
		{
			return device;
		}
	}

	/* was there any device we could switch to our depth? */
	if (new_mode)
	{
		/* should this alert go on the destination monitor, or would that be hard to notice? */
		switch (myAlert(alrtCHANGE_DEPTH, get_general_filter_upp()))
		{
			case iOK: /* change now */
				error= SetDepth(changeable_device, new_mode, 1, new_type);
				if (error==noErr)
				{
					return changeable_device;
				}
				
			case iCANCEL: /* quit */
				exit(0);
		}
	}
	
	return (GDHandle) NULL;
}

void LowLevelSetEntries(
	short start,
	short count,
	CSpecArray aTable)
{
	OSErr error;
	CntrlParam pb;
	VDSetEntryRecord parameters;

	GDHandle device= GetGDevice();

	parameters.csStart= start;
	parameters.csCount= count;
	parameters.csTable= aTable;

	pb.ioCompletion= (ProcPtr) NULL;
	pb.ioVRefNum= 0;
	pb.ioCRefNum= (*device)->gdRefNum;
	pb.csCode= (*device)->gdType==directType ? cscDirectSetEntries : cscSetEntries;
	*((Ptr*)&pb.csParam)= (Ptr) &parameters;
	
	error= PBControl((ParmBlkPtr)&pb, FALSE);
	vassert(error==noErr, csprintf(temporary, "Control(cscXSetEntries, ...) returned #%d", error));
	
	return;
}

/* ---------- private code */

static boolean parse_device(
	GDHandle device,
	short depth,
	GDHandle *changeable_device,
	short *new_mode,
	short *new_type)
{
	PixMapHandle pixmap;
	short mode;
	
	if (TestDeviceAttribute(device, screenDevice)&&TestDeviceAttribute(device, screenActive))
	{
		pixmap= (*device)->gdPMap;
		if ((*pixmap)->pixelType==chunky&&(*pixmap)->pixelSize==depth)
		{
			return TRUE;
		}
		else
		{
			/* can this monitor do the given depth in color?  if it can, pick it above a
				previous monitor that can only do grays */
			mode= HasDepth(device, depth, 1, 1);
			if (mode&&(!*new_mode||!*new_type))
			{
				*changeable_device= device;
				*new_mode= mode;
				*new_type= 1;
			}
			else
			{
				if (!*new_mode)
				{
					/* can it do grayscale?  (as a last resort) */
					mode= HasDepth(device, depth, 1, 0);
					if (mode)
					{
						*changeable_device= device;
						*new_mode= mode;
						*new_type= 0;
					}
				}
			}
			
			return FALSE;
		}
	}
}
