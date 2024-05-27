/*
DEVICES.C
Wednesday, September 1, 1993 12:09:55 PM

Friday, August 12, 1994 10:23:19 AM
	added LowLevelSetEntries().
Wednesday, December 7, 1994 12:03:05 PM  (Jason')
	added GDSpec stuff, changed BestDevice.  this is sort of the last nail in the coffin of
	pathways ever being compiled easily again.
*/

#include "macintosh_cseries.h"

#include "textures.h"

#include <Video.h>
#include <Slots.h>

#ifndef env68k
#include "RequestVideo.c"
#endif

#ifdef mpwc
#pragma segment modules
#endif

/* ---------- constants */

/* ---------- globals */

/* for menu bar hiding */
static boolean menu_bar_hidden= FALSE;
static short old_menu_bar_height;
static RgnHandle old_gray_region;

/* ---------- private code */

/* ---------- code */

void HideMenuBar(
	GDHandle device)
{
	RgnHandle device_region;
	static boolean first_time= TRUE;
	
	// <6/17/96 AMR> Commented out the following because it's called every time the depth
	// or resolution is changed to resquare the corners of the main device.
//	assert(!menu_bar_hidden);

	if (device==GetMainDevice())
	{
		Rect device_bounds= (*device)->gdRect;
		
		if (first_time)
		{
			first_time= FALSE;
			
			old_menu_bar_height= LMGetMBarHeight();
			LMSetMBarHeight(0);
		
			old_gray_region= NewRgn();
			CopyRgn(LMGetGrayRgn(), old_gray_region);
		}
		
		device_region= NewRgn();
		RectRgn(device_region, &device_bounds);
		UnionRgn(LMGetGrayRgn(), device_region, LMGetGrayRgn());
		DisposeRgn(device_region);
		
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
		CopyRgn(old_gray_region, LMGetGrayRgn());
		DisposeRgn(old_gray_region);
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
	for (device= GetDeviceList(); device; device= GetNextDevice(device))
	{
		if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
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

/* returns the device best matching the given GDSpec, if any, making the GDSpec more explicit if
	necessary */
GDHandle BestDevice(
	GDSpecPtr device_spec)
{
	GDHandle best_device= (GDHandle) NULL;
	GDSpec spec;

	/* try the given GDSpec, then search the entire device list */
	if (HasDepthGDSpec(device_spec)) best_device= MatchGDSpec(device_spec);
	if (!best_device)
	{
		GDHandle device;
		
		for (device= GetDeviceList(); device && !best_device; device= GetNextDevice(device))
		{
			if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
			{
				BuildExplicitGDSpec(&spec, device, device_spec->flags, device_spec->bit_depth, 0, 0);
				if (HasDepthGDSpec(&spec)) *device_spec= spec, best_device= device;
			}
		}
	}

	/* if we failed to find anything in color, try in grayscale */
	if (!best_device && (device_spec->flags&deviceIsColor))
	{
		spec= *device_spec;
		spec.flags&= ~deviceIsColor;
		
		if (best_device= BestDevice(&spec)) *device_spec= spec;
	}

	return best_device;
}

void LowLevelSetEntries(
	short start,
	short count,
	CSpecArray aTable)
{
	OSErr error;
	CntrlParam pb;
	VDSetEntryRecord parameters;

	if (count)
	{
		GDHandle device= GetGDevice();
	
		parameters.csStart= start;
		parameters.csCount= count;
		parameters.csTable= (ColorSpec *) StripAddress((Ptr)aTable);
	
		memset(&pb, 0, sizeof(CntrlParam));
		pb.ioCompletion= (IOCompletionUPP) NULL;
		pb.ioVRefNum= 0;
		pb.ioCRefNum= (*device)->gdRefNum;
		pb.csCode= (*device)->gdType==directType ? cscDirectSetEntries : cscSetEntries;
		*((Ptr*)&pb.csParam)= (Ptr) &parameters;
		
		error= PBControl((ParmBlkPtr)&pb, FALSE);
		vwarn(error==noErr, csprintf(temporary, "Control(cscXSetEntries, ...) returned #%d;g;", error));
	}	
	return;
}

short GetSlotFromGDevice(
	GDHandle device)
{
	AuxDCEHandle DCE= (AuxDCEHandle) GetDCtlEntry((*device)->gdRefNum);
	
	assert(DCE);
	
	return DCE ? (*DCE)->dCtlSlot : 0;
}

OSErr GetNameFromGDevice(
	GDHandle device,
	char *name)
{
	OSErr error;
    SpBlockPtr mySpBPtr= (SpBlockPtr) NewPtrClear(sizeof(SpBlock));

	if (mySpBPtr)
	{
		mySpBPtr->spSlot= GetSlotFromGDevice(device);
		mySpBPtr->spID= 0;
		mySpBPtr->spExtDev= 0;
		mySpBPtr->spCategory= 1; // Get the board sResource
		mySpBPtr->spCType= 0;
		mySpBPtr->spDrvrSW= 0;
		mySpBPtr->spDrvrHW= 0;
		mySpBPtr->spTBMask= 0;
		
		error= SNextTypeSRsrc(mySpBPtr);
		if (error==noErr)
		{
			mySpBPtr->spID= 2; // get the description string (spSPointer was set up by NextTypeSRsrc)
			error= SGetCString(mySpBPtr);
			if (error==noErr)
			{
				strcpy(name, (char *) mySpBPtr->spResult);
				DisposPtr((Ptr) mySpBPtr->spResult);
				c2pstr(name);
			}
		}
		
		DisposePtr((Ptr)mySpBPtr);
	}
	else
	{
		error= MemError();
	}
    
    return error;
}

/* based on explicit state provided */
void BuildExplicitGDSpec(
	GDSpecPtr device_spec,
	GDHandle device,
	word flags,
	short bit_depth,
	short width,
	short height)
{
	assert(!(flags&~deviceIsColor));
	assert(bit_depth==8||bit_depth==16||bit_depth==32);
	
	device_spec->slot= device ? GetSlotFromGDevice(device) : NONE;
	device_spec->flags= flags;
	device_spec->bit_depth= bit_depth;
	device_spec->width= width;
	device_spec->height= height;
	
	return;
}

/* based on current state */
void BuildGDSpec(
	GDSpecPtr device_spec,
	GDHandle device)
{
	assert(device);
	
	device_spec->slot= GetSlotFromGDevice(device);
	device_spec->flags= (*device)->gdFlags&deviceIsColor;
	device_spec->bit_depth= (*(*device)->gdPMap)->pixelSize;
	device_spec->width= RECTANGLE_WIDTH(&(*device)->gdRect);
	device_spec->height= RECTANGLE_HEIGHT(&(*device)->gdRect);
	
	return;
}

Boolean EqualGDSpec(
	GDSpecPtr device_spec1,
	GDSpecPtr device_spec2)
{
	Boolean equal= FALSE;
	
	if (MatchGDSpec(device_spec1)==MatchGDSpec(device_spec2))
	{
		if (device_spec1->flags==device_spec2->flags && device_spec1->bit_depth==device_spec2->bit_depth)
		{
			// does not compare screen dimensions
			
			equal= TRUE;
		}
	}
	
	return equal;
}

boolean machine_has_display_manager(
	void)
{
#ifndef env68k
	boolean displayMgrPresent;
	SpBlock spBlock;
	long value= 0;
	OSErr err;

	err= Gestalt(gestaltDisplayMgrAttr, &value);
	displayMgrPresent= !err && (value&(1<<gestaltDisplayMgrPresent));
	displayMgrPresent= displayMgrPresent && (SVersion(&spBlock)==noErr);	// need slot manager

	// fuck this. We must have DMGetDisplayMode to restore everything just right.
	// Let the speds using pre-7.5.3 install Display Enabler 2.0.2.
	err= Gestalt(gestaltDisplayMgrVers, (long*)&value);
	displayMgrPresent= displayMgrPresent && !err && (value >= 0x00020000);
	
	return displayMgrPresent;
#else
	return FALSE;
#endif
}

void SetResolutionGDSpec(
	GDSpecPtr device_spec,
	VDSwitchInfoPtr switchInfo)
{
#ifndef env68k
	if (machine_has_display_manager())
	{
		GDHandle device= MatchGDSpec(device_spec);
	
		if (device_spec->width && device_spec->height)
		{
			// device manager
			VideoRequestRec record;
			OSErr error;
			
			memset(&record, 0, sizeof(VideoRequestRec));
			
			record.screenDevice= device;
			record.reqBitDepth= device_spec->bit_depth;
			record.reqHorizontal= device_spec->width;
			record.reqVertical= device_spec->height;
	
//			dprintf("requesting settings for record @%p (device %p)", &record, device);
//			GetRequestTheDM1Way(&record, device);
			error= RVRequestVideoSetting(&record);
	
			if (switchInfo)
			{
				record.switchInfo= *switchInfo;	// they want a specific mode
			}
//			dprintf("trying to use display manager to go to (#%d,#%d)", device_spec->width, device_spec->height);
			error= RVSetVideoRequest(&record);
			if (error==noErr)
			{
			}
			
			if (error!=noErr) dprintf("Eric’s crazy code returned #%d", error);
		}
	}
#endif
	
	return;
}

/* returns the given GDevice to the state specified by the GDSpec */
void SetDepthGDSpec(
	GDSpecPtr device_spec)
{
	GDHandle device= MatchGDSpec(device_spec);
	
	if (device)
	{
		short mode= HasDepth(device, device_spec->bit_depth, deviceIsColor, device_spec->flags);
		
		if (mode)
		{
			SetDepth(device, mode, deviceIsColor, device_spec->flags);
		}
	}
	
	return;
}

boolean HasDepthGDSpec(
	GDSpecPtr device_spec)
{
	GDHandle device= MatchGDSpec(device_spec);
	boolean has_depth= FALSE;
	
	if (device)
	{
		if (HasDepth(device, device_spec->bit_depth, deviceIsColor, device_spec->flags))
		{
			has_depth= TRUE;
		}
	}
	
	return has_depth;
}

/* returns the GDHandle referenced by the given GDSpec */
GDHandle MatchGDSpec(
	GDSpecPtr device_spec)
{
	GDHandle device, matched_device= (GDHandle) NULL;
	
	for (device= GetDeviceList(); device; device= GetNextDevice(device))
	{
		if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
		{
			if (device_spec->slot==GetSlotFromGDevice(device))
			{
				matched_device= device;
				break;
			}
		}
	}
	
	return matched_device;
}

CTabHandle build_macintosh_color_table(
	struct color_table *color_table)
{
	CTabHandle macintosh_color_table= (CTabHandle) NewHandle(sizeof(ColorTable)+color_table->color_count*sizeof(ColorSpec));

	if (macintosh_color_table)
	{
		struct rgb_color *color;
		ColorSpec *spec;
		short i;
		
		(*macintosh_color_table)->ctFlags= 0;
		(*macintosh_color_table)->ctSeed= GetCTSeed();
		(*macintosh_color_table)->ctSize= color_table->color_count-1;
		
		color= color_table->colors;
		spec= (*macintosh_color_table)->ctTable;
		for (i= 0; i<color_table->color_count; ++i, ++spec, ++color)
		{
			spec->value= i;
			spec->rgb= *((RGBColor *) color);
		}
		
		CTabChanged(macintosh_color_table);
	}

	return macintosh_color_table;
}

struct color_table *build_color_table(
	struct color_table *color_table,
	CTabHandle macintosh_color_table)
{
	struct rgb_color *color;
	ColorSpec *spec;
	short i;
	
	color_table->color_count= (*macintosh_color_table)->ctSize+1;
	
	color= color_table->colors;
	spec= (*macintosh_color_table)->ctTable;
	for (i= 0; i<color_table->color_count; ++i, ++spec, ++color)
	{
		*color= *((struct rgb_color *)&spec->rgb);
	}
	
	return color_table;
}
